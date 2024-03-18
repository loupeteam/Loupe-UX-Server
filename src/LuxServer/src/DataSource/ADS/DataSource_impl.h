#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H

#include <memory>
#include <string>
#include <map>

#ifdef WIN32
#include <AdsLib.h>
#include "AdsVariable.h"
#else
#include <ads/AdsLib.h>
#include <ads/AdsVariable.h>
#endif
#include "AdsExtension.h"

#include "SymbolParser.h"
#include "DataParser.h"
#include "util.h"
namespace lux{
class adsdatasrc_impl {
    crow::json::wvalue& find(std::string symbolName, crow::json::wvalue& datasource);
    void parseSymbols(void* pSymbols, unsigned int nSymSize);
    void parseDatatypes(void* pDatatypes, unsigned int nDTSize);
    bool supportType(ULONG flags);

public:
    bool readingProps = false;
    std::vector<std::string> propertyReads;
    long readInfo();
    // void cacheDataTypes();
    bool cacheSymbolInfo(std::string symbolName);
    void parseBuffer(crow::json::wvalue& variable, symbolMetadata& datatype, void* pBuffer, unsigned long size);
    bool encodeBuffer(std::string& variable, void* pBuffer, std::string& value, unsigned long size);

    adsdatasrc_impl(){}
    ~adsdatasrc_impl();
    long nErr;
    std::shared_ptr<AdsDevice> route = nullptr;

    std::unordered_map<std::string, dataType_member_base*> dataTypes;
    crow::json::wvalue symbolData;
    symbolMetadata symbolInfo;

    symbolMetadata& findInfo(std::string& symbolName);
    crow::json::wvalue& findValue(std::string& symbolName);

    bool getMemberInfo(std::string targetSymbol, CAdsSymbolInfo Entry);
    bool getMemberInfo(std::string       targetSymbol,
                       PAdsDatatypeEntry Entry,
                       std::string       prefix,
                       unsigned long     group,
                       uint32_t          offset);

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
    uint32_t indexGroup;   // index group in ADS server interface
    uint32_t indexOffset;      // index offset in ADS server interface
    uint32_t length;       // count of bytes to read
};
}
#endif // ADSDATASRC_IMPL_H
