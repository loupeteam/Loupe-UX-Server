#ifndef DATASRC_H
#define DATASRC_H

#include <string>
#include "crow_all.h"


class DataSource {
  /* data */
public:

  virtual void updateVariables(std::vector<std::string> symbolNames){ };
  virtual crow::json::wvalue getVariable(std::string symbolName){ return crow::json::wvalue{}; };
  
};

#endif // ADSDATASRC_H