#ifndef JSONSERVER_H
#define JSONSERVER_H
#include <future>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "../DataSource/datasrc.h"

class jsonRequest
{
public:
  std::string type;
  crow::websocket::connection *conn;
  std::vector<std::string> keys;    
  jsonRequest(crow::websocket::connection &conn, std::string type, std::vector<std::string> keys) : conn(&conn), type(type), keys(keys) {};
  jsonRequest(){};
};

class plcrequest{
public:
  std::string type;
  std::unordered_map<std::string, bool> keys;    
  plcrequest(std::string type, std::unordered_map<std::string, bool> keys) : type(type), keys(keys) {};
  plcrequest(){};
};

class jsonserver
{
private:
  void * app;
  std::future<void> thread;
  std::mutex mtx_connections;
  std::mutex mtx_send;
  std::unordered_set<void*> users;
  std::vector<DataSource*> dataSources;
  std::mutex mtx_pendingReadRequests;
  std::deque<jsonRequest> pendingReadRequests;
  std::deque<jsonRequest> pendingWriteRequests;

  /* data */
public:
  jsonserver(/* args */);
  ~jsonserver();
  int start(int port, bool async = true);
  int stop();
  int addDataSource(DataSource &ds);
  int readVariables( const std::vector<std::string> &keys );
  int handlePendingRequests(  );
  int sendResponse( crow::websocket::connection *conn, const std::vector<std::string> &keys );
  void addPendingReadRequest( jsonRequest &req );
  bool getPendingReadRequest( jsonRequest *req );
  void addConnection( crow::websocket::connection &conn );
  void removeConnection( crow::websocket::connection &conn );
  void serverThread();
  crow::json::wvalue getVariable(std::string);  
};

#endif // JSONSERVER