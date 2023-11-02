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
    thread = std::async(std::launch::async, &jsonserver::serverThread, this);

    if (async) {
        thread = pApp->port(port)
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
    std::unordered_map<void*, jsonRequest> currentRequest;

    //Generate a consolidated packet
    jsonRequest r;
    while (getPendingReadRequest(&r)) {
        if (r.keys.size() + variables.size() < 500) {
            for ( auto key : r.keys) {
                variables[ key ] = true;
            }
            auto request = currentRequest[r.conn];
            if (request.conn == nullptr) {
                //Add to current request
                currentRequest[r.conn] = r;
            } else {
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

        measureTime("Packet preparation time: ", start);

        //Send the request to the PLC
        this->readVariables(keys);

        //Measure the response time
        start = std::chrono::high_resolution_clock::now();

        //Send the response to the clients
        for ( auto r : currentRequest) {
            this->sendResponse(r.second.conn, r.second.keys);
            measureTime("Total response time: ", r.second.receiveTime);
        }

        measureTime("Response time: ", start);
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

    measureTime("Packet json preparation time: ", start);

    //Lock the user list
    std::lock_guard<std::mutex> _(mtx_connections);

    //Check that the connection is still in the user list
    if (users.find(conn) == users.end()) {
        return 0;
    }
    //Measure string generation time
    start = getTimestamp();
    std::string text = x.dump();

    measureTime("String generation time: ", start);

    //Measure the send time
    start = getTimestamp();

    conn->send_text(text);

    measureTime("Send time: ", start);

    return 0;
}

crow::json::wvalue jsonserver::getVariable(std::string name)
{
    return this->dataSources.at(0)->getSymbolValue(name);
}
