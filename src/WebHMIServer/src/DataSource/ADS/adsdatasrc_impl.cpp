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

void adsdatasrc_impl::readInfo(){

  AdsSymbolUploadInfo2 info;
  long nResult = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_UPLOADINFO2, 0,
                                sizeof(info), &info);

  if (nResult == ADSERR_NOERR) {
    // size of symbol information
    PBYTE pSym = new BYTE[info.nSymSize];
    PBYTE pDT = new BYTE[info.nDatatypeSize];

    if (pSym && pDT) {
        // upload symbols (instances)
        long resultSym = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_UPLOAD, 0,
                                info.nSymSize, pSym);
        // get size of datatype description
        // upload datatye-descriptions
        long resultDt = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_DT_UPLOAD, 0,
                                info.nDatatypeSize, pDT);

        this->parsedSymbols = new CAdsParseSymbols(pSym, info.nSymSize, pDT, info.nDatatypeSize);
      }
      delete[] pSym;
      delete[] pDT;
    }

}

void adsdatasrc_impl::cacheDataTypes(){

    UINT count = this->parsedSymbols->DatatypeCount();
    for (int i = 0; i < count; i++) {
      PAdsDatatypeEntry pAdsDatatypeEntry = this->parsedSymbols->GetTypeByIndex(i);
      string typeName = PADSDATATYPENAME(pAdsDatatypeEntry);
      dataType_member_base *dt = this->getType(typeName);
      if (dt->valid) {
        continue;
      }
      dt->name = typeName;
    }
}

void adsdatasrc_impl::cacheSymbolInfo( std::string symbolName ){
  CAdsSymbolInfo Entry;
  this->parsedSymbols->Symbol(symbolName, Entry);
  this->getMemberInfo(Entry);
}

void adsdatasrc_impl::parseBuffer(crow::json::wvalue &variable, string &datatype, void *pBuffer, unsigned long size ){
  
  byte *buffer = (byte*)pBuffer;

  dataType_member_base* info = this->getType( datatype );
  
  if(!info->valid){
    info->valid = true;
    this->PopulateChildren(info);
  }

  //If this is a basic data type, then we can parse it with the given parser
  //Otherwise go through the members and parse them
  if( info->baseType ){
    info->parse( variable, buffer, size );
    return;
  }
  else{
    for ( auto member : info->members )
    {
      parseBuffer( variable[member->name], member->type, buffer + member->offset, member->size );
    }
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

//    std::cout << "No Keys";
    
  }   
  return *ret;
}

void adsdatasrc_impl::populateSymbolInfo(crow::json::wvalue &symbol,
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

PAdsSymbolEntry adsdatasrc_impl::populateSymbolInfo(crow::json::wvalue &symbol,
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