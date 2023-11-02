#include "jsonserver.h"
#include "crow_all.h"
#include "util.h"

jsonserver::jsonserver() {}

jsonserver::~jsonserver() {}

int jsonserver::stop()
{
    crow::SimpleApp* app = (crow::SimpleApp*)this->app;
    app->stop();

    delete app;
    return 0;
}

int jsonserver::start(int port, bool async)
{
    auto pApp = new crow::SimpleApp();
    this->app = pApp;

    CROW_WEBSOCKET_ROUTE((*pApp), "/")
    .onopen([&](crow::websocket::connection& conn) {
        CROW_LOG_INFO << "new websocket connection from "
                      << conn.get_remote_ip();
        this->addConnection(conn);
    })
    .onclose(
        [&](crow::websocket::connection& conn, const std::string& reason) {
        CROW_LOG_INFO << "websocket connection closed: " << reason;
        this->removeConnection(conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                   bool is_binary) {
        crow::json::rvalue message = crow::json::load(data);

        std::string type = message["type"].s();

        if (type == "write") {
            mtx_pendingReadRequests.lock();

            crow::json::rvalue variables = message["data"];
            try {
                this->dataSources.at(0)->writeSymbolValue(variables);
            } catch (...) {
                std::cout << "Error writing symbol value" << std::endl;
            }
            //Send the response
            crow::json::wvalue x;
            x["type"] = "writeresponse";
            x["data"] = crow::json::wvalue("{}");

            conn.send_text(x.dump());
            mtx_pendingReadRequests.unlock();
        } else if (type == "read") {
            crow::json::rvalue variables = message["data"];
            std::vector<std::string> keys;
            //Go through all the array values and add them to the response
            for (auto& v : variables) {
                std::string key = v.s();
                keys.push_back(key);
            }
            this->addPendingReadRequest(jsonRequest(conn, "read", keys, std::chrono::steady_clock::now()));
        }
    });

    //Start a new thread for the server updates
    serverThreads = std::async(std::launch::async, &jsonserver::serverThread, this);

    //Spin up a few response threads
    for (int i = 0; i < 5; i++) {
        responseThreads.push_back(std::async(std::launch::async, &jsonserver::responseThread, this));
    }

    if (async) {
        crowThreads = pApp->port(port)
                      .concurrency(5)
                      .run_async();
    } else {
        pApp->port(port)
        .concurrency(5)
        .run();
    }
    return 0;
}

void jsonserver::addConnection(crow::websocket::connection& conn)
{
    std::lock_guard<std::mutex> _(mtx_connections);
    users.insert(&conn);
}
void jsonserver::removeConnection(crow::websocket::connection& conn)
{
    std::lock_guard<std::mutex> _(mtx_connections);
    mtx_pendingReadRequests.lock();
    for ( auto it = pendingReadRequests.begin(); it != pendingReadRequests.end(); ) {
        if (it->conn == &conn) {
            it = pendingReadRequests.erase(it);
        } else {
            ++it;
        }
    }
    //Keep pending write requests, but mark them as no response
    for ( auto it = pendingWriteRequests.begin(); it != pendingWriteRequests.end(); ++it ) {
        if (it->conn == &conn) {
            it->conn = nullptr;
        }
    }
    users.erase(&conn);
    mtx_pendingReadRequests.unlock();
}

void jsonserver::serverThread()
{
    while (1) {
        handlePendingRequests();
    }
}

void jsonserver::responseThread()
{
    while (1) {
        //Send the response to the clients
        mtx_pendingResponse.lock();
        //Check if the response returned is valid
        if (pendingResponse.size() > 0) {
            jsonRequest response = pendingResponse.front();
            pendingResponse.pop_front();
            mtx_pendingResponse.unlock();
            response.sendResponseTime = getTimestamp();
            this->sendResponse(response.conn, response.keys);
            response.finishResponseTime = getTimestamp();
            response.printTimes();
        } else {
            mtx_pendingResponse.unlock();
        }
    }
}

int jsonserver::addDataSource(DataSource& ds)
{
    this->dataSources.push_back(&ds);
    return 0;
}

int jsonserver::readVariables(const std::vector<std::string>& keys)
{
    this->dataSources.at(0)->readSymbolValue(keys);
    return 0;
}

void jsonserver::addPendingReadRequest(jsonRequest& req)
{
    mtx_pendingReadRequests.lock();
    pendingReadRequests.push_back(req);
    mtx_pendingReadRequests.unlock();
}
bool jsonserver::getPendingReadRequest(jsonRequest* req)
{
    mtx_pendingReadRequests.lock();
    while (pendingReadRequests.size() > 0) {
        //Get the oldest request
        *req = pendingReadRequests.front();
        pendingReadRequests.pop_front();

        //Check if the packet is really old, throw it out if it is
        if (std::chrono::steady_clock::now() - req->receiveTime > std::chrono::milliseconds(3000)) {
            std::cout << "Packet too old, throwing it out. Age: " <<
                (std::chrono::steady_clock::now() - req->receiveTime).count() << std::endl;
            continue;
        }

        mtx_pendingReadRequests.unlock();
        return true;
    }
    mtx_pendingReadRequests.unlock();
    return false;
}

int jsonserver::handlePendingRequests()
{
    //Measure the time for the request handling
    auto start = getTimestamp();

    //Build up a consolidate PLC read request
    std::unordered_map<std::string, bool> variables;
    variables.reserve(500);

    //Keep track of the current request for each connection
    std::unordered_map<void*, jsonRequest> currentRequest;

    //Generate a consolidated packet
    jsonRequest r;

    while (getPendingReadRequest(&r)) {
        if (r.keys.size() + variables.size() < 500) {
            for ( auto key : r.keys) {
                variables[ key ] = true;
            }
            auto& request = currentRequest[r.conn];
            if (request.conn == nullptr) {
                //Add to current request
                currentRequest[r.conn] = r;
                request.pickedTime = getTimestamp();
            } else {
                //If we already have a request for this connection, add the keys to it
                // to ensure that we don't send the same request twice, or miss variables that
                // are requested in a previous request
                request.keys.insert(request.keys.end(), r.keys.begin(), r.keys.end());
            }
        } else {
            //No room, put it back
            pendingReadRequests.push_front(r);
            break;
        }
    }

    //Check if any of the keys are children of other keys
    std::vector<std::string> keysToRemove;
    for ( auto kv : variables ) {
        for ( auto kv2 : variables ) {
            //If the key is not the same and the key starts with the other key, remove it
            //But only if the longer one has a dot or a bracket after the shorter one
            if ((kv.first != kv2.first) && (kv.first.find(kv2.first) == 0)) {
                if (kv.first.size() > kv2.first.size()) {
                    if ((kv.first[kv2.first.size()] == '.') || (kv.first[kv2.first.size()] == '[')) {
                        keysToRemove.push_back(kv.first);
                    }
                }
            }
        }
    }
    for ( auto key : keysToRemove ) {
        variables.erase(key);
    }

    //Send the request if there are any
    if (variables.size() > 0) {
        //Generatate the list of keys
        std::vector<std::string> keys;
        for (auto kv : variables) {
            keys.push_back(kv.first);
        }

        for ( auto& kv : currentRequest) {
            kv.second.sendRequestTime = getTimestamp();
        }

        //Send the request to the PLC
        this->readVariables(keys);

        mtx_pendingResponse.lock();
        for ( auto& kv : currentRequest) {
            kv.second.finishRequestTime = getTimestamp();
            pendingResponse.push_back(kv.second);
        }
        mtx_pendingResponse.unlock();
    }
    return 0;
}

int jsonserver::sendResponse(crow::websocket::connection* conn, const std::vector<std::string>& keys)
{
    if (conn == nullptr) {
        return 0;
    }

    //Measure packet json preparation time
    auto start = getTimestamp();

    //Prepare the response before checking if the connection is still in the user list
    //This minimizes the time that the mutex is locked
    crow::json::wvalue::list variablesList;
    variablesList.reserve(keys.size());
    for (auto& v : keys) {
        std::string key = v;
        crow::json::wvalue variable;
        variable[key] = this->getVariable(key);
        variablesList.push_back(variable);
    }
    crow::json::wvalue x;
    x["type"] = "readresponse";
    x["data"] = crow::json::wvalue(variablesList);

    //Lock the user list
    std::lock_guard<std::mutex> _(mtx_connections);

    //Check that the connection is still in the user list
    if (users.find(conn) == users.end()) {
        return 0;
    }
    //Measure string generation time
    start = getTimestamp();
    std::string text = x.dump();

    //Measure the send time
    start = getTimestamp();

    conn->send_text(text);

    return 0;
}

crow::json::wvalue jsonserver::getVariable(std::string name)
{
    return this->dataSources.at(0)->getSymbolValue(name);
}

void jsonRequest::printTimes()
{
    //Print the delta times
    std::cout << "\n Delta times:" << std::endl;
    printTime("Time waiting for request pick : ", receiveTime, pickedTime);
    printTime("Time to Generate Variables    : ", pickedTime, sendRequestTime);
    printTime("Time to talk to PLC           : ", sendRequestTime, finishRequestTime);
    printTime("Time waiting for response pick: ", finishRequestTime, sendResponseTime);
    printTime("Time to respond               : ", sendResponseTime, finishResponseTime);
    printTime("Total response time:          : ", receiveTime, finishResponseTime);
}
