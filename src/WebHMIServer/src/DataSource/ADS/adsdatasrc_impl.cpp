#include "./adsdatasrc_impl.h"

#include "adsdatasrc.h"
#include "util.h"
#include <unordered_map>
using namespace std;


void adsdatasrc_impl::getMemberInfo( CAdsSymbolInfo main ){

		if ( !main.m_pEntry )
			main.m_pEntry = parsedSymbols->GetTypeByName(main.type);

    dataType_member_base *dt = this->getType(main.type);
    if(!dt->valid) {
      dt->valid = true;
      this->PopulateChildren(dt);
      if ( main.m_pEntry ){
        getMemberInfo( main.m_pEntry, main.fullname, main.iGrp, main.iOffs );
      }  
    }
}

void adsdatasrc_impl::getMemberInfo( PAdsDatatypeEntry Entry, string prefix, unsigned long group, uint32_t offset ){
  CAdsSymbolInfo info;
  for (size_t i = 0; i < parsedSymbols->SubSymbolCount(Entry) ; i++)
  {
    AdsDatatypeEntry SubEntry;
    parsedSymbols->SubSymbolInfo( Entry, i, info );
    parsedSymbols->SubSymbolEntry( Entry, i, SubEntry );            
    if(prefix != ""){
      info.fullname = prefix + "." + info.name;
    }
    else{
      info.fullname = info.name;
    }
    crow::json::wvalue &symbol = findInfo( info.fullname );

    populateSymbolInfo( symbol, info.fullname, group, offset, info);

    if(SubEntry.subItems == 0){
			info.m_pEntry = parsedSymbols->GetTypeByName(info.type);
      if(info.m_pEntry){
        getMemberInfo( info.m_pEntry, info.fullname, group, offset + SubEntry.offs);
      }
    }
    else{
      getMemberInfo( &SubEntry, info.fullname, group, offset + SubEntry.offs );
    }    
  }    
}

void adsdatasrc_impl::PopulateChildren( dataType_member_base * parser ){  
      PAdsDatatypeEntry pAdsDatatypeEntry = this->parsedSymbols->GetTypeByName(parser->type);      
      parser->isArray = pAdsDatatypeEntry->arrayDim > 0;
      for (int j = 0;j < this->parsedSymbols->SubSymbolCount(pAdsDatatypeEntry); j++) {
        CAdsSymbolInfo info;
        this->parsedSymbols->SubSymbolInfo(pAdsDatatypeEntry, j, info);
        if( info.isProperty){
          auto member = datatype_member::parserForType(info.name, info.type, info.offs, info.size);
          parser->members.push_back(member);
          PopulateChildren(member);
        }
        else{
          auto member = datatype_member::parserForType(info.name, info.type, info.offs, info.size);
          parser->members.push_back(member);
          PopulateChildren(member);
        }
      }
}

dataType_member_base * dataType_member_base::parserForType( std::string name, std::string type, unsigned long iOffs, unsigned long size ){
      if(type._Starts_with("STRING")){
        return new dataType_member_string(name, type, iOffs, size);
      }
      else if(type._Starts_with("WSTRING")){
        return new dataType_member_wstring(name, type, iOffs, size);
      }
      else if(type == "BOOL" || type == "BIT" || type == "BIT8"){
        return new dataType_member_bool(name, type, iOffs, size);
      }
      else if(type == "BYTE" || type == "USINT" || type == "BITARR8" || type == "UINT8"){
        return new dataType_member_base_typed<uint8_t>{name, type, iOffs, size};
      }
      else if(type == "SINT" || type == "INT8" ){
        return new dataType_member_base_typed<int8_t>{name, type, iOffs, size};
      }
      else if(type == "UINT" || type == "WORD" || type == "BITARR16" || type == "UINT16"){
        return new dataType_member_base_typed<uint16_t>{name, type, iOffs, size};
      }
      else if(type == "INT" || type == "INT16"){
        return new dataType_member_base_typed<int16_t>{name, type, iOffs, size};
      }
      else if(type == "ENUM" ){
        return new dataType_member_enum{name, type, iOffs, size};
      }
      else if(type == "DINT" || type == "INT32"){
        return new dataType_member_base_typed<int32_t>{name, type, iOffs, size};
      }
      else if(type == "UDINT" || type == "DWORD" || type == "TIME" || type == "TIME_OF_DAY" || type == "TOD" || type == "BITARR32" || type == "UINT32"){
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
      }
      else if(type == "DATE_AND_TIME" || type == "DT" || type == "DATE" ){
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
      }
      else if(type == "REAL" || type == "FLOAT"){
        return new dataType_member_base_typed<float>{name, type, iOffs, size};
      }
      else if(type == "DOUBLE" || type == "LREAL" ){
        return new dataType_member_base_typed<double>{name, type, iOffs, size};
      }
      else if(type == "LWORD" || type == "ULINT" || type == "LTIME" || type == "UINT64" ){
        return new dataType_member_base_typed<uint64_t>{name, type, iOffs, size};
      }
      else if(type == "LINT" || type == "INT64" ){
        return new dataType_member_base_typed<int64_t>{name, type, iOffs, size};
      }
      else if(type._Starts_with("POINTER") ){
        return new dataType_member_pointer{name, type, iOffs, size};
      }
      else{
        return new datatype_member(name, type, iOffs, size);
      }
      
}

dataType_member_base* adsdatasrc_impl::getType( std::string& typeName ){
  dataType_member_base *dt = this->dataTypes[typeName];
  if( dt == NULL ){
    dt = dataType_member_base::parserForType("", typeName, 0, 0);
    dt->valid = false;
    this->dataTypes[typeName] = dt;
  }
  return dt;
}

crow::json::wvalue& adsdatasrc_impl::findInfo(std::string& symbolName){
  crow::json::wvalue & ret = find(symbolName, this->symbolInfo, true);  
  if( ret.keys().size() == 0 ){
    find(symbolName, this->symbolData, false);  
  }
  return ret;
}

crow::json::wvalue& adsdatasrc_impl::findValue(std::string& symbolName){
  crow::json::wvalue & ret = find(symbolName, this->symbolData, false);  
  if( ret.keys().size() == 0 ){
    find(symbolName, this->symbolInfo, true);
  }
  return ret;
}

crow::json::wvalue& adsdatasrc_impl::find(std::string symbolName, crow::json::wvalue &datasource, bool value){
  toLower(symbolName);
  std::vector<std::string> path = split(symbolName, '.');
  crow::json::wvalue *ret = &datasource[path[0]];
  for( int i = 1; i < path.size(); i++){
    crow::json::wvalue *data;
    if(value){
       data = &(*ret)["value"];       
    }
    else{
        data = ret;
    }
    ret = &(*data)[path[i]];
  }
  //Check if the value has anything in it
  if( ret->keys().size() == 0 ){

    std::cout << "No Keys";
    
  }   
  return *ret;
}

void populateSymbolInfo(crow::json::wvalue &symbol,
                                   std::string &symbolName,
                                   unsigned long parentGroup,
                                   unsigned long parentOffset,
                                   CAdsSymbolInfo &info) {
  symbol["name"] = symbolName;
  symbol["group"] = parentGroup;
  symbol["offset"] = parentOffset + info.offs;
  symbol["size"] = info.size;
  symbol["type"] = info.type;
  symbol["comment"] = info.comment;
}

PAdsSymbolEntry populateSymbolInfo(crow::json::wvalue &symbol,
                                   std::string &symbolName,
                                   PAdsSymbolEntry pAdsSymbolEntry) {
  symbol["name"] = symbolName;
  symbol["group"] = pAdsSymbolEntry->iGroup;
  symbol["offset"] = pAdsSymbolEntry->iOffs;
  symbol["size"] = pAdsSymbolEntry->size;
  symbol["type"] = PADSSYMBOLTYPE(pAdsSymbolEntry);
  symbol["comment"] = PADSSYMBOLCOMMENT(pAdsSymbolEntry);
  return PADSNEXTSYMBOLENTRY(pAdsSymbolEntry);
}