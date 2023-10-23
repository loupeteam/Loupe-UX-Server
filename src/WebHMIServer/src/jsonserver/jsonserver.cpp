#include "jsonserver.h"
#include "crow_all.h"

jsonserver::jsonserver() {}

jsonserver::~jsonserver() {}
int jsonserver::stop() {
  crow::SimpleApp *app = (crow::SimpleApp *)this->app;
  app->stop();

  delete app;
  return 0;
}

int jsonserver::start( int port, bool async ) {

  auto pApp = new crow::SimpleApp();
  this->app = pApp;

  CROW_WEBSOCKET_ROUTE((*pApp), "/")
      .onopen([&](crow::websocket::connection &conn) {
        CROW_LOG_INFO << "new websocket connection from "
                      << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
//        this->users.insert(&conn);
      })
      .onclose(
          [&](crow::websocket::connection &conn, const std::string &reason) {
            CROW_LOG_INFO << "websocket connection closed: " << reason;
            std::lock_guard<std::mutex> _(mtx);
//            users.erase(&conn);
          })
      .onmessage([&](crow::websocket::connection &conn, const std::string &data,
                     bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        crow::json::rvalue message = crow::json::load(data);
        crow::json::rvalue variables = message["data"];

        crow::json::wvalue x;
        x["type"] = "readresponse";
        crow::json::wvalue::list variablesList;
        //Go through all the array values and add them to the response
        for (auto &v : variables) {
          std::string key = v.s();
          crow::json::wvalue variable;
          variable[key] = this->getVariable(key);
          variablesList.push_back(variable);
        }
        x["data"] = crow::json::wvalue(variablesList);
        conn.send_text(x.dump());
      });

  if( async){
    thread = pApp->port(port)
    .run_async();
  }
  else{
    pApp->port(port).run();    
  }
  return 0;
}

int jsonserver::addDataSource(DataSource &ds){
  this->dataSources.push_back(&ds);
  return 0;
}
crow::json::wvalue jsonserver::getVariable(std::string name){
  return this->dataSources.at(0)->getVariable(name);
}