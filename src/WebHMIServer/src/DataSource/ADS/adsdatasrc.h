#ifndef ADSDATASRC_H
#define ADSDATASRC_H
#include "../datasrc.h"
#include <string>

class adsdatasrc : public DataSource{
private:
    void * _impl;
public:
  adsdatasrc(/* args */);
  ~adsdatasrc();

  void readSymbolValue( std::string symbolName );
  void readSymbolValue( std::vector<std::string> symbolNames );
  void readPlcData();
  bool ready();
  crow::json::wvalue getSymbolValue(std::string symbolName);  
};

#endif // ADSDATASRC_H