#include "./adsdatasrc_impl.h"

#include "adsdatasrc.h"
#include "util.h"
#include <unordered_map>
using namespace std;


adsdatasrc_impl::~adsdatasrc_impl() {
  delete this->parsedSymbols;
  for (auto &dt : this->dataTypes) {
    delete dt.second;
  }
}

void adsdatasrc_impl::getMemberInfo( std::string targetSymbol, CAdsSymbolInfo main ){

		if ( !main.m_pEntry )
			main.m_pEntry = parsedSymbols->GetTypeByName(main.type);

    if ( main.m_pEntry ){
      getMemberInfo(targetSymbol, main.m_pEntry, main.fullname, main.iGrp, main.iOffs );
    }  
}

void adsdatasrc_impl::getMemberInfo( std::string targetSymbol, PAdsDatatypeEntry Entry, string prefix, unsigned long group, uint32_t offset ){

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

    //If the shorter of target symbol and current symbol is not in the other one, then skip
    // This is so we don't cache large strcutres that are not needed
    if( targetSymbol.size() < info.fullname.size() ){
      if( targetSymbol.compare(0, targetSymbol.size(), info.fullname, 0, targetSymbol.size()) != 0 ){
        continue;
      }
    }
    else{
      if( info.fullname.compare(0, info.fullname.size(), targetSymbol, 0, info.fullname.size()) != 0 ){
        continue;
      }
    }    

    symbolMetadata &symbol = this->symbolInfo[info.fullname];

    populateSymbolInfo( symbol, info.fullname, group, offset, info);

    if(SubEntry.subItems == 0){
			info.m_pEntry = parsedSymbols->GetTypeByName(info.type);
      if(info.m_pEntry){
        getMemberInfo( targetSymbol, info.m_pEntry, info.fullname, group, offset + SubEntry.offs);
      }
    }
    else{
      getMemberInfo( targetSymbol, &SubEntry, info.fullname, group, offset + SubEntry.offs );
    }    
  }    
}

void adsdatasrc_impl::prepareDatatypeParser( dataType_member_base * parser ){  
      PAdsDatatypeEntry pAdsDatatypeEntry = this->parsedSymbols->GetTypeByName(parser->type);      
      parser->isArray = pAdsDatatypeEntry->arrayDim > 0;
      for (int j = 0;j < this->parsedSymbols->SubSymbolCount(pAdsDatatypeEntry); j++) {
        CAdsSymbolInfo info;
        this->parsedSymbols->SubSymbolInfo(pAdsDatatypeEntry, j, info);
        auto member = datatype_member::parserForType(info.name, info.type, info.offs, info.size);
        parser->members.push_back(member);
      }
      parser->valid = true;
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
        this->ready = true;
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
  symbolMetadata &info = this->symbolInfo[Entry.name];
  populateSymbolInfo( info, Entry.fullname, Entry);
  this->getMemberInfo(symbolName, Entry);

}

void adsdatasrc_impl::parseBuffer(crow::json::wvalue &variable, string &datatype, void *pBuffer, unsigned long size ){
  
  byte *buffer = (byte*)pBuffer;

  dataType_member_base* dt = this->getType( datatype );
  
  if(!dt->valid){
    this->prepareDatatypeParser(dt);
  }

  //If this is a basic data type, then we can parse it with the given parser
  //Otherwise go through the members and parse them
  if( dt->baseType ){
    dt->parse( variable, buffer, size );
    return;
  }
  else{
    for ( auto member : dt->members )
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

symbolMetadata & adsdatasrc_impl::findInfo(std::string& symbolName){
  symbolMetadata & info = this->symbolInfo[symbolName];
  if( info.valid == false ){
    cacheSymbolInfo( symbolName );
  }
  return info;
}

crow::json::wvalue& adsdatasrc_impl::findValue(std::string& symbolName){
  crow::json::wvalue & ret = find(symbolName, this->symbolData, false);  
  if( ret.keys().size() == 0 ){
//    find(symbolName, this->symbolInfo, true);
  }
  return ret;
}

crow::json::wvalue& adsdatasrc_impl::find(std::string symbolName, crow::json::wvalue &datasource, bool value){
  toLower(symbolName);
  std::deque<std::string> path = split(symbolName, '.');
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

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata &symbol,
                                   std::string &symbolName,
                                   unsigned long parentGroup,
                                   unsigned long parentOffset,
                                   CAdsSymbolInfo &info) {
  symbol.name = symbolName;
  symbol.group = parentGroup;
  symbol.gOffset = parentOffset + info.offs;
  symbol.offset = info.offs;
  symbol.size = info.size;
  symbol.type = info.type;
  symbol.comment = info.comment;
  symbol.valid = true;
}

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata &symbol,
                                   std::string &symbolName,
                                   CAdsSymbolInfo &info) {
  symbol.name = symbolName;
  symbol.group = info.iGrp;
  symbol.gOffset = info.iOffs;
  symbol.offset = info.offs;
  symbol.size = info.size;
  symbol.type = info.type;
  symbol.comment = info.comment;
  symbol.valid = true;
}

PAdsSymbolEntry adsdatasrc_impl::populateSymbolInfo(symbolMetadata &symbol,
                                   std::string &symbolName,
                                   PAdsSymbolEntry pAdsSymbolEntry) {                                    
  symbol.name = symbolName;
  symbol.group = pAdsSymbolEntry->iGroup;
  symbol.gOffset = pAdsSymbolEntry->iOffs;
  symbol.offset = 0;
  symbol.size = pAdsSymbolEntry->size;
  symbol.type = PADSSYMBOLTYPE(pAdsSymbolEntry);
  symbol.comment = PADSSYMBOLCOMMENT(pAdsSymbolEntry);
  symbol.valid = true;
  return PADSNEXTSYMBOLENTRY(pAdsSymbolEntry);
}