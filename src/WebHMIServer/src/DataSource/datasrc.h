#ifndef DATASRC_H
#define DATASRC_H

#include <string>
#include "crow_all.h"


class DataSource {
  /* data */
public:

  virtual crow::json::wvalue getVariable(std::string symbolName){};
  
};

#endif // ADSDATASRC_H