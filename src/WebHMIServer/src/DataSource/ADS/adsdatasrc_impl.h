#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H

#include <memory>
#include <string>
typedef int BOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;

#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "crow_all.h"

#include "AdsParseSymbols.h"
#include "adsparser.h"
#include "util.h"

class symbolMetadata {
  std::unordered_map<std::string,symbolMetadata> _members;
public:
  std::string name;
  bool valid = false;
  unsigned long group = 0;
  unsigned long gOffset = 0;
  unsigned long offset = 0;
  unsigned long size = 0;
  std::string type;  
  std::string comment;
  symbolMetadata(){};
  //Override the [] operator to return a reference to the member
  symbolMetadata& operator[](const std::string& key) {
    std::deque<std::string> path = split(key, '.');
    if( path.size() == 1 ){
      return _members[key];
    }
    else{
      std::string member = path.front();
      path.pop_front();
      return _members[member][path];
    }
  }
  symbolMetadata& operator[](std::deque<std::string> path) {
    std::string key = path.front();
    if( path.size() == 1 ){
      return _members[path[0]];
    }
    else{
      path.pop_front();
      return _members[key][path];
    }
  }

};

class adsdatasrc_impl {

  crow::json::wvalue &find(std::string symbolName,
                           crow::json::wvalue &datasource, bool value);
  void parseSymbols(void* pSymbols, unsigned int nSymSize );
  void parseDatatypes(void* pDatatypes, unsigned int nDTSize);
                    
public:
  void readInfo();
  void cacheDataTypes();
  void cacheSymbolInfo( std::string symbolName );
  void parseBuffer(crow::json::wvalue &variable, std::string &datatype,
                   void *buffer, unsigned long size);

  adsdatasrc_impl(){};
  ~adsdatasrc_impl();
  long nErr, nPort;
  AmsAddr Addr;
  PAmsAddr pAddr = &Addr;

  std::unordered_map<std::string, dataType_member_base *> dataTypes;
  crow::json::wvalue symbolData;
  symbolMetadata symbolInfo;

  symbolMetadata &findInfo(std::string &symbolName);
  crow::json::wvalue &findValue(std::string &symbolName);
  dataType_member_base *getType(std::string &typeName);

  void getMemberInfo( std::string targetSymbol, CAdsSymbolInfo Entry);
  void getMemberInfo( std::string targetSymbol,
                      PAdsDatatypeEntry Entry, 
                      std::string prefix,
                      unsigned long group, 
                      uint32_t offset);

  void prepareDatatypeParser( dataType_member_base * dataType );
  void populateSymbolInfo(symbolMetadata &symbol, std::string &symbolName,
                        unsigned long parentGroup, unsigned long parentOffset,
                        CAdsSymbolInfo &info);

  void populateSymbolInfo(symbolMetadata &symbol,
                          std::string &symbolName,
                          CAdsSymbolInfo &info);

  PAdsSymbolEntry populateSymbolInfo(symbolMetadata &symbol,
                                   std::string &symbolName,
                                   PAdsSymbolEntry pAdsSymbolEntry);

  CAdsParseSymbols *parsedSymbols = NULL;
  bool ready = false;
};


typedef struct dataPar
{
  unsigned long		indexGroup;	// index group in ADS server interface
  unsigned long		indexOffset;	// index offset in ADS server interface
  unsigned long		length;		// count of bytes to read
};

#endif // ADSDATASRC_IMPL_H