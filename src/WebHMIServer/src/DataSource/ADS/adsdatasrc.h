#ifndef ADSDATASRC_H
#define ADSDATASRC_H
#include "../datasrc.h"
#include <string>

class adsdatasrc : public DataSource{
private:
    void * _impl;
    void ParseSymbols(void* pSymbols, unsigned int nSymSize );
    void ParseDatatypes(void* pDatatypes, unsigned int nDTSize);
public:
  adsdatasrc(/* args */);
  ~adsdatasrc();

  void getGlobalSymbolInfo();
  void getDatatypeInfo();
  void getSymbolInfo( std::string symbolName );
  void readSymbolValue( std::string symbolName );
};

#endif // ADSDATASRC_H