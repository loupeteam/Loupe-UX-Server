#ifndef JSONSERVER_H
#define JSONSERVER_H
#include <future>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "../DataSource/datasrc.h"
class jsonserver
{
private:
  void * app;
  std::future<void> thread;
  std::mutex mtx;
  std::unordered_set<void*> users;
  std::vector<DataSource*> dataSources;

  /* data */
public:
  jsonserver(/* args */);
  ~jsonserver();
  int start(int port, bool async = true);
  int stop();
  int addDataSource(DataSource &ds);
  crow::json::wvalue getVariable(std::string);  
};

#endif // JSONSERVER