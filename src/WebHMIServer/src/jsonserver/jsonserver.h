#ifndef JSONSERVER_H
#define JSONSERVER_H
#include <future>
#include <mutex>
#include <unordered_set>
#include "../DataSource/datasrc.h"
class jsonserver
{
private:
  void * app;
  std::future<void> thread;
  std::mutex mtx;
  std::unordered_set<void*> users;
  std::unordered_set<DataSource*> dataSources;

  /* data */
public:
  jsonserver(/* args */);
  ~jsonserver();
  int start(int port);
  int stop();
  int addDataSource(DataSource &ds);
};

#endif // JSONSERVER