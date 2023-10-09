#ifndef DATASRC_H
#define DATASRC_H

#include <string>


class DataSource {
  /* data */
public:

  virtual void getSymbolInfo(std::string symbolName){};
  
};

#endif // ADSDATASRC_H