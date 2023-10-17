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

int jsonserver::start( int port ) {

  auto pApp = new crow::SimpleApp();

  CROW_WEBSOCKET_ROUTE((*pApp), "/")
      .onopen([&](crow::websocket::connection &conn) {
        CROW_LOG_INFO << "new websocket connection from "
                      << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        this->users.insert(&conn);
      })
      .onclose(
          [&](crow::websocket::connection &conn, const std::string &reason) {
            CROW_LOG_INFO << "websocket connection closed: " << reason;
            std::lock_guard<std::mutex> _(mtx);
            users.erase(&conn);
          })
      .onmessage([&](crow::websocket::connection &conn, const std::string &data,
                     bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        crow::json::wvalue x = crow::json::load(data);
        x["type"] = "readresponse";
        crow::json::wvalue variable = x["data"];
        for (size_t i = 0; i < variable.keys().size(); i++)
        {
//          x["data"] = variable.
        }
        

        conn.send_text(x.dump());
      });

  thread = pApp->port(port)
  .run_async();
  this->app = pApp;
  return 0;
}

int jsonserver::addDataSource(DataSource &ds){
  this->dataSources.push_back(&ds);
  return 0;
}
crow::json::wvalue jsonserver::getVariable(std::string name){
  return this->dataSources.at(0)->getVariable(name);
}