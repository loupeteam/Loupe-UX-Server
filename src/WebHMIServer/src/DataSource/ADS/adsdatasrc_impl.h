#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H


#include <string.h>

typedef int BOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;

#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "crow_all.h"


#include "AdsParseSymbols.h"


class dataType_member_impl{
public:
    std::string name;
	std::string	type;
    unsigned long offset;
    unsigned long size;
    dataType_member_impl(std::string name, std::string type, unsigned long offset, unsigned long size) : name(name), type(type), offset(offset), size(size) {};
};

class dataType_impl{
public:
    bool valid;
    std::string name;
    std::vector<dataType_member_impl> members;  
};

class adsdatasrc_impl {
    crow::json::wvalue& find(std::string symbolName, crow::json::wvalue &datasource, bool value);    
public:
    adsdatasrc_impl(){};
    long nErr, nPort; 
    AmsAddr              Addr; 
    PAmsAddr             pAddr = &Addr; 
    AdsSymbolUploadInfo  tAdsSymbolUploadInfo; 
    
    std::unordered_map<std::string, dataType_impl> dataTypes;
    crow::json::wvalue   symbolData;
    crow::json::wvalue   symbolInfo;
    crow::json::wvalue& findInfo(std::string& symbolName);    
    crow::json::wvalue& findValue(std::string& symbolName);    
    dataType_impl& getType( std::string& typeName );
    void getMemberInfo( CAdsSymbolInfo Entry );
    void getMemberInfo( PAdsDatatypeEntry Entry, std::string prefix );

//    CAdsParseSymbols *m_pDynSymbols;
    PBYTE m_pSymbols = NULL;
    PBYTE m_pDatatypes = NULL;
    PPAdsSymbolEntry m_ppSymbolArray = NULL;
    PPAdsDatatypeEntry m_ppDatatypeArray = NULL;
    CAdsParseSymbols * parsedSymbols = NULL;
};

#endif // ADSDATASRC_IMPL_H