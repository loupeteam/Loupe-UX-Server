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
  long nErr, nPort;
  AmsAddr Addr;
  PAmsAddr pAddr = &Addr;

  std::unordered_map<std::string, dataType_member_base *> dataTypes;
  crow::json::wvalue symbolData;
  crow::json::wvalue symbolInfo;
  crow::json::wvalue &findInfo(std::string &symbolName);
  crow::json::wvalue &findValue(std::string &symbolName);
  dataType_member_base *getType(std::string &typeName);

  void getMemberInfo(CAdsSymbolInfo Entry);
  void getMemberInfo(PAdsDatatypeEntry Entry, std::string prefix,
                     unsigned long group, uint32_t offset);

  void PopulateChildren( dataType_member_base * dataType );
  void populateSymbolInfo(crow::json::wvalue &symbol, std::string &symbolName,
                        unsigned long parentGroup, unsigned long parentOffset,
                        CAdsSymbolInfo &info);

  PAdsSymbolEntry populateSymbolInfo(crow::json::wvalue &symbol,
                                   std::string &symbolName,
                                   PAdsSymbolEntry pAdsSymbolEntry);

  CAdsParseSymbols *parsedSymbols = NULL;
};


typedef struct dataPar
{
  unsigned long		indexGroup;	// index group in ADS server interface
  unsigned long		indexOffset;	// index offset in ADS server interface
  unsigned long		length;		// count of bytes to read
};

#endif // ADSDATASRC_IMPL_H