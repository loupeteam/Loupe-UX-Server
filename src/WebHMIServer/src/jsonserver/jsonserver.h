#ifndef JSONSERVER_H
#define JSONSERVER_H
#include <future>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "../DataSource/datasrc.h"

class jsonRequest {
public:
    std::chrono::steady_clock::time_point receiveTime;
    std::chrono::steady_clock::time_point pickedTime;
    std::chrono::steady_clock::time_point sendRequestTime;
    std::chrono::steady_clock::time_point finishRequestTime;
    std::chrono::steady_clock::time_point sendResponseTime;
    std::chrono::steady_clock::time_point finishResponseTime;
    std::string type;
    crow::websocket::connection* conn = nullptr;
    std::vector<std::string> keys;
    jsonRequest(crow::websocket::connection& conn, std::string type, std::vector<std::string> keys) : conn(&conn), type(
            type), keys(keys) {}
    jsonRequest(crow::websocket::connection&          conn,
                std::string                           type,
                std::vector<std::string>              keys,
                std::chrono::steady_clock::time_point receiveTime) : conn(&conn), type(type), keys(keys), receiveTime(
            receiveTime)
    {}
    jsonRequest(){}
    void printTimes();
};

class plcrequest {
public:
    std::string type;
    std::unordered_map<std::string, bool> keys;
    plcrequest(std::string type, std::unordered_map<std::string, bool> keys) : type(type), keys(keys) {}
    plcrequest(){}
};

class jsonserver {
private:
    void* app;
    std::future<void> serverThreads;
    std::vector<std::future<void> > responseThreads;
    std::future<void> crowThreads;
    std::mutex mtx_connections;
    std::mutex mtx_send;
    std::unordered_set<void*> users;
    std::vector<DataSource*> dataSources;

    std::deque<jsonRequest> pendingWriteRequests;

    std::mutex mtx_pendingReadRequests;
    std::deque<jsonRequest> pendingReadRequests;

    std::mutex mtx_pendingResponse;
    std::deque<jsonRequest> pendingResponse;

    /* data */
public:
    jsonserver(/* args */);
    ~jsonserver();
    int start(int port, bool async = true);
    int stop();

    int addDataSource(DataSource* ds);
    int readVariables(const std::vector<std::string>& keys);
    int handlePendingRequests();
    int handlePendingResponses();
    int sendResponse(crow::websocket::connection* conn, const std::vector<std::string>& keys);
    void addPendingReadRequest(const jsonRequest& req);
    bool getPendingReadRequest(jsonRequest* req);
    void addConnection(crow::websocket::connection& conn);
    void removeConnection(crow::websocket::connection& conn);
    void serverThread();
    void responseThread();
    crow::json::wvalue getVariable(std::string);
};

#endif // JSONSERVER
