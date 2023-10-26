#ifndef DATASRC_H
#define DATASRC_H

#include <string>
#include "crow_all.h"


class DataSource {
  /* data */
public:

  virtual void readSymbolValue( std::string symbolName ){ };
  virtual void readSymbolValue(std::vector<std::string> symbolNames){ };
  virtual crow::json::wvalue getSymbolValue(std::string symbolName){ return crow::json::wvalue{}; };
  
};

#endif // ADSDATASRC_H