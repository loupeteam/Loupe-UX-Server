#ifndef ADSDATASRC_IMPL_H
#define ADSDATASRC_IMPL_H

#include <codecvt>
#include <memory>
#include <string.h>
typedef int BOOL;
typedef unsigned long ULONG;
typedef ULONG *PULONG;

#include "TcAdsDef.h"
#include "TcAdsAPI.h"
#include "crow_all.h"

#include "AdsParseSymbols.h"
class dataType_member_base {
public:
  bool valid = false;
  bool isArray = false;
  bool isMember = false;
  std::string name;
  std::string type;
  unsigned long offset;
  unsigned long size;
  std::vector<dataType_member_base *> members;
  dataType_member_base(std::string name, std::string type, unsigned long offset,
                       unsigned long size)
      : name(name), type(type), offset(offset), size(size){};
  dataType_member_base(){};
  virtual bool parse(crow::json::wvalue &variable, void *buffer,
                     unsigned long size) {
    return false;
  }
  static dataType_member_base *parserForType(std::string name, std::string type,
                                             unsigned long iOffs,
                                             unsigned long size);
};
class datatype_member : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  datatype_member(std::string name, std::string type, unsigned long offset,
                  unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *pbuffer, unsigned long size) {
    BYTE *buffer = (BYTE *)pbuffer;
    if(isArray){
      for (auto &member : members) {
        member->parse(variable[ std::stoi(member->name)], buffer + member->offset, member->size);
      }
      return true;
    }
    else if (members.size() > 0) {
      for (auto &member : members) {
        member->parse(variable[member->name], buffer + member->offset, member->size);
      }
      return true;
    } else {
      variable = "NOP: " + type;
      return false;
    }
  }
};

template <typename T>
class dataType_member_base_typed : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_base_typed(std::string name, std::string type,
                             unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (size == sizeof(T)) {
      if (isMember) {
        variable[name] = *(T *)buffer;
      } else {
        variable = *(T *)buffer;
      }
      return true;
    }
    return false;
  }
};

class dataType_member_bool : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_bool(std::string name, std::string type,
                             unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (size == sizeof(bool)) {
      if (isMember) {
        variable[name] = (bool)(*(BYTE*)buffer & 0x01);
      } else {
        variable = (bool)(*(BYTE*)buffer & 0x01);
      }
      return true;
    }
    return false;
  }
};

class dataType_member_string : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_string(std::string name, std::string type,
                         unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (size > 0) {
      if (isMember) {
        variable[name] = std::string((char *)buffer);
      } else {
        variable = std::string((char *)buffer);
      }
      return true;
    }
    return false;
  }
};

class dataType_member_enum : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_enum(std::string name, std::string type, unsigned long offset,
                       unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (size > 0) {
      if (isMember) {
        variable[name] = *(uint32_t *)buffer;
      } else {
        variable = *(uint32_t *)buffer;
      }
      return true;
    }
    return false;
  }
};

class dataType_member_wstring : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_wstring(std::string name, std::string type,
                          unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (size > 0) {
      // Convert from a wstring to a string.
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      std::string value = converter.to_bytes((wchar_t *)buffer);
      if (isMember) {
        variable[name] = value;
      } else {
        variable = value;
      }
      return true;
    }
    return false;
  }
};

class dataType_member_unsupported : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_unsupported(std::string name, std::string type,
                              unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (isMember) {
      variable[name] = "UNSUPPORTED: " + type;
    } else {
      variable = "UNSUPPORTED: " + type;
    }
    return true;
  }
};
class dataType_member_pointer : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType_member_pointer(std::string name, std::string type,
                          unsigned long offset, unsigned long size)
      : dataType_member_base(name, type, offset, size){};
  bool parse(crow::json::wvalue &variable, void *buffer,
             unsigned long size) override {
    if (isMember) {
      variable[name] = "Pointer to: " + type;
    } else {
      variable = "Pointer to: " + type;
    }
    return true;
  }
};

class dataType : public dataType_member_base {
public:
  // Constructor with name, type, offset and size
  dataType(std::string type, unsigned long size)
      : dataType_member_base(name = "", type, offset = 0, size){};
  dataType(){};
  std::string name;

  bool parse(crow::json::wvalue &variable, void *pbuffer,
             unsigned long size) override {
    BYTE *buffer = (BYTE *)pbuffer;
    if (members.size() > 0) {
      for (auto &member : members) {
        member->parse(variable[member->name], buffer + member->offset, member->size);
      }
      return true;
    } else {
      return false;
    }
  }
};

class adsdatasrc_impl {
  crow::json::wvalue &find(std::string symbolName,
                           crow::json::wvalue &datasource, bool value);

public:
  adsdatasrc_impl(){};
  long nErr, nPort;
  AmsAddr Addr;
  PAmsAddr pAddr = &Addr;
  AdsSymbolUploadInfo tAdsSymbolUploadInfo;

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

  PBYTE m_pSymbols = NULL;
  PBYTE m_pDatatypes = NULL;
  PPAdsSymbolEntry m_ppSymbolArray = NULL;
  PPAdsDatatypeEntry m_ppDatatypeArray = NULL;
  CAdsParseSymbols *parsedSymbols = NULL;
};

void populateSymbolInfo(crow::json::wvalue &symbol, std::string &symbolName,
                        unsigned long parentGroup, unsigned long parentOffset,
                        CAdsSymbolInfo &info);

PAdsSymbolEntry populateSymbolInfo(crow::json::wvalue &symbol,
                                   std::string &symbolName,
                                   PAdsSymbolEntry pAdsSymbolEntry);

typedef struct dataPar
{
  unsigned long		indexGroup;	// index group in ADS server interface
  unsigned long		indexOffset;	// index offset in ADS server interface
  unsigned long		length;		// count of bytes to read
};

#endif // ADSDATASRC_IMPL_H