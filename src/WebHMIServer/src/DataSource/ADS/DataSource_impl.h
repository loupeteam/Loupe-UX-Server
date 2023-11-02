#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H

#include <memory>
#include <string>
typedef int BOOL;
typedef unsigned long ULONG;
typedef ULONG* PULONG;

#include <TcAdsDef.h>
#include <TcAdsAPI.h>
#include "crow_all.h"

#include "SymbolParser.h"
#include "DataParser.h"
#include "util.h"

class adsdatasrc_impl {
    crow::json::wvalue& find(std::string symbolName, crow::json::wvalue& datasource);
    void parseSymbols(void* pSymbols, unsigned int nSymSize);
    void parseDatatypes(void* pDatatypes, unsigned int nDTSize);
    bool supportType(ULONG flags);
public:
    long readInfo();
    // void cacheDataTypes();
    bool cacheSymbolInfo(std::string symbolName);
    void parseBuffer(crow::json::wvalue& variable, symbolMetadata& datatype, void* pBuffer, unsigned long size);
    bool encodeBuffer(std::string& variable, void* pBuffer, std::string& value, unsigned long size);

    adsdatasrc_impl(){}
    ~adsdatasrc_impl();
    long nErr, nPort;
    AmsAddr Addr;
    AmsAddr* pAddr = &Addr;

    std::unordered_map<std::string, dataType_member_base*> dataTypes;
    crow::json::wvalue symbolData;
    symbolMetadata symbolInfo;

    symbolMetadata& findInfo(std::string& symbolName);
    crow::json::wvalue& findValue(std::string& symbolName);
    // dataType_member_base* getType(std::string& typeName);

    void getMemberInfo(std::string targetSymbol, CAdsSymbolInfo Entry);
    void getMemberInfo(std::string       targetSymbol,
                       PAdsDatatypeEntry Entry,
                       std::string       prefix,
                       unsigned long     group,
                       uint32_t          offset);

    // void prepareDatatypeParser(dataType_member_base* dataType);
    void populateSymbolInfo(symbolMetadata& symbol, std::string& symbolName,
                            unsigned long parentGroup, unsigned long parentOffset,
                            CAdsSymbolInfo& info);

    void populateSymbolInfo(symbolMetadata& symbol,
                            std::string&    symbolName,
                            CAdsSymbolInfo& info);

    PAdsSymbolEntry populateSymbolInfo(symbolMetadata& symbol,
                                       std::string&    symbolName,
                                       PAdsSymbolEntry pAdsSymbolEntry);

    CAdsParseSymbols* parsedSymbols = NULL;
    bool ready = false;
};

struct dataPar {
    unsigned long indexGroup;   // index group in ADS server interface
    unsigned long indexOffset;      // index offset in ADS server interface
    unsigned long length;       // count of bytes to read
};

#endif // ADSDATASRC_IMPL_H
