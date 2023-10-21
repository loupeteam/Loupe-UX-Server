#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H


#include <string.h>
#include <codecvt>

typedef int BOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;

#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "crow_all.h"


#include "AdsParseSymbols.h"


class dataType_member_base{
public:
    std::string name;
	std::string	type;
    unsigned long offset;
    unsigned long size;
    dataType_member_base(std::string name, std::string type, unsigned long offset, unsigned long size) : name(name), type(type), offset(offset), size(size) {};
    virtual bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size ){
        return false;
    }
};
class  datatype_member : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    datatype_member(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size ){
        return false;
    }
};


template <typename T>
class dataType_member_base_typed : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_base_typed(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        if( size == sizeof(T) ){
            variable[name] = *(T*)buffer;
            return true;
        }
        return false;
    }
};

class dataType_member_string : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_string(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        if( size > 0 ){
            variable[name] = std::string((char*)buffer);
            return true;
        }
        return false;
    }
};

class dataType_member_enum : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_enum(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        if( size > 0 ){
            variable[name] = *(uint32_t*)buffer;
            return true;
        }
        return false;
    }
};

class dataType_member_wstring : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_wstring(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        if( size > 0 ){
            //Convert from a wstring to a string.
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            variable[name] = converter.to_bytes((wchar_t*)buffer);
            return true;
        }
        return false;
    }
};

class dataType_member_unsupported : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_unsupported(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        variable[name] = "UNSUPPORTED: " + type;
        return true;
    }
};
class dataType_member_pointer : public dataType_member_base{
public:
    //Constructor with name, type, offset and size
    dataType_member_pointer(std::string name, std::string type, unsigned long offset, unsigned long size) : dataType_member_base(name, type, offset, size) {};
    bool parse(crow::json::wvalue &variable, void *buffer, unsigned long size) override {
        variable[name] = "Pointer to: " + type;
        return true;
    }
};


class dataType{
public:
    bool valid;
    std::string name;
    std::vector<dataType_member_base*> members;  
};

class adsdatasrc_impl {
    crow::json::wvalue& find(std::string symbolName, crow::json::wvalue &datasource, bool value);    
public:
    adsdatasrc_impl(){};
    long nErr, nPort; 
    AmsAddr              Addr; 
    PAmsAddr             pAddr = &Addr; 
    AdsSymbolUploadInfo  tAdsSymbolUploadInfo; 
    
    std::unordered_map<std::string, dataType> dataTypes;
    crow::json::wvalue   symbolData;
    crow::json::wvalue   symbolInfo;
    crow::json::wvalue& findInfo(std::string& symbolName);    
    crow::json::wvalue& findValue(std::string& symbolName);    
    dataType& getType( std::string& typeName );
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