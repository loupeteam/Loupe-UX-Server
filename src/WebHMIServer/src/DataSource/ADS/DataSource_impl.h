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

class symbolMetadata {
    std::unordered_map<std::string, symbolMetadata> _members;
public:
    std::string name;
    bool valid = false;
    bool notFound = false;
    unsigned long group = 0;
    unsigned long gOffset = 0;
    unsigned long offset = 0;
    unsigned long size = 0;
    unsigned long flags = 0;
    unsigned long dataType;

    datatype_flags_struct flags_struct;

    std::string type;
    std::string comment;
    symbolMetadata(){}
    //Iterator for members
    typedef std::unordered_map<std::string, symbolMetadata>::iterator iterator;
    iterator begin() { return _members.begin(); }
    iterator end() { return _members.end(); }

    //Member count
    size_t memberCount() { return _members.size(); }
    //Get the members
    std::unordered_map<std::string, symbolMetadata>& members() { return _members; }

    //Override the [] operator to return a reference to the member
    symbolMetadata& operator[](const std::string& key)
    {
        //Split the path into a deque
        std::deque<std::string> path = split(key, std::string(".["));
        if (path.size() == 1) {
            return _members[key];
        } else {
            std::string member = path.front();
            path.pop_front();
            return _members[member][path];
        }
    }
    symbolMetadata& operator[](std::deque<std::string> path)
    {
        std::string key = path.front();
        if (path.size() == 1) {
            return _members[path[0]];
        } else {
            path.pop_front();
            return _members[key][path];
        }
    }
};

class adsdatasrc_impl {
    crow::json::wvalue& find(std::string symbolName, crow::json::wvalue& datasource);
    void parseSymbols(void* pSymbols, unsigned int nSymSize);
    void parseDatatypes(void* pDatatypes, unsigned int nDTSize);
    bool supportType(ULONG flags);
public:
    long readInfo();
    void cacheDataTypes();
    bool cacheSymbolInfo(std::string symbolName);
    void parseBuffer(crow::json::wvalue& variable, std::string& datatype, void* buffer, unsigned long size);
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
    dataType_member_base* getType(std::string& typeName);

    void getMemberInfo(std::string targetSymbol, CAdsSymbolInfo Entry);
    void getMemberInfo(std::string       targetSymbol,
                       PAdsDatatypeEntry Entry,
                       std::string       prefix,
                       unsigned long     group,
                       uint32_t          offset);

    void prepareDatatypeParser(dataType_member_base* dataType);
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

typedef struct dataPar {
    unsigned long indexGroup;   // index group in ADS server interface
    unsigned long indexOffset;      // index offset in ADS server interface
    unsigned long length;       // count of bytes to read
};

#endif // ADSDATASRC_IMPL_H
