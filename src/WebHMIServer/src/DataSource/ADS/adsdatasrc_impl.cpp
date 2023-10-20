#include "./adsdatasrc_impl.h"

#include "adsdatasrc.h"
#include "util.h"

using namespace std;


void adsdatasrc_impl::getMemberInfo( CAdsSymbolInfo main ){

		if ( !main.m_pEntry )
			main.m_pEntry = parsedSymbols->GetTypeByName(main.type);
    if ( main.m_pEntry ){
      getMemberInfo( main.m_pEntry, main.fullname );
    }
}

void adsdatasrc_impl::getMemberInfo( PAdsDatatypeEntry Entry, string prefix ){
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

    if(SubEntry.subItems == 0){
			info.m_pEntry = parsedSymbols->GetTypeByName(info.type);
      if(info.m_pEntry){
        getMemberInfo( info.m_pEntry, info.fullname );
      }
    }
    else{
      getMemberInfo( &SubEntry, info.fullname );
    }    
  }    
}


dataType_impl& adsdatasrc_impl::getType( std::string& typeName ){
  dataType_impl *dt = &this->dataTypes[typeName];
  if( dt == NULL ){
    dt = &this->dataTypes.emplace(typeName, dataType_impl{}).first->second;
    dt->name = typeName;
    dt->valid = false;



  }
  return *dt;
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
    find(symbolName, this->symbolData, false);
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
  return *ret;
}