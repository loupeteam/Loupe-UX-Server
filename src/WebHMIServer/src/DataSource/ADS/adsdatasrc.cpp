#include "./adsdatasrc.h"
#include "./adsdatasrc_impl.h"

#include <iostream>
#include "adsdatasrc.h"

#define impl ((adsdatasrc_impl *)_impl)

using namespace std;

adsdatasrc_impl * static_impl;

adsdatasrc::adsdatasrc() {

  _impl = new adsdatasrc_impl();
  impl->nPort = AdsPortOpen();
  impl->Addr.netId.b[0] = 192;
  impl->Addr.netId.b[1] = 168;
  impl->Addr.netId.b[2] = 0;
  impl->Addr.netId.b[3] = 46;
  impl->Addr.netId.b[4] = 1;
  impl->Addr.netId.b[5] = 1;

  impl->pAddr->port = 851;

  static_impl = impl;
}

adsdatasrc::~adsdatasrc() {
  impl->nErr = AdsPortClose();
  if (impl->nErr)
    cerr << "Error: AdsPortClose: " << impl->nErr << '\n';
  delete impl;
}

//Converts string to lower in place
void toLower( string &str){
  std::transform(str.begin(), str.end(), str.begin(),[](unsigned char c){ return std::tolower(c); });
}
//Returns an array of strings split by delim
std::vector<std::string> split( string &str, char delim){
  std::vector<std::string> elems;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

PAdsSymbolEntry populuteSymbolInfo( crow::json::wvalue &symbol, std::string& symbolName, PAdsSymbolEntry pAdsSymbolEntry){
    symbol["name"] = symbolName;
    symbol["group"] = pAdsSymbolEntry->iGroup;
    symbol["offset"] = pAdsSymbolEntry->iOffs;
    symbol["size"] = pAdsSymbolEntry->size;
    symbol["type"] = PADSSYMBOLTYPE(pAdsSymbolEntry);
    symbol["comment"] = PADSSYMBOLCOMMENT(pAdsSymbolEntry);
    return PADSNEXTSYMBOLENTRY(pAdsSymbolEntry);    
}

void adsdatasrc::getGlobalSymbolInfo() {
  char *pchSymbols = NULL; 

    impl->nErr = AdsSyncReadReq(impl->pAddr, 
      ADSIGRP_SYM_UPLOADINFO, 
      0x0, 
      sizeof(impl->tAdsSymbolUploadInfo), 
      &impl->tAdsSymbolUploadInfo);
    if (impl->nErr) cerr << "Error: AdsSyncReadReq: " << impl->nErr << '\n'; 

    pchSymbols = new char[ impl->tAdsSymbolUploadInfo.nSymSize ];
    //assert(pchSymbols);
    long nErr;

    // Read information of the PLC-variable
    nErr = AdsSyncReadReq( impl->pAddr, 
        ADSIGRP_SYM_UPLOAD, 
        0,
        impl->tAdsSymbolUploadInfo.nSymSize, 
        pchSymbols);

    if (nErr) cerr << "Error: AdsSyncReadReq: " << nErr << '\n';

    // Print out the information of the PLC-variable
    PAdsSymbolEntry pAdsSymbolEntry = (PAdsSymbolEntry)pchSymbols; 

    int uiIndex;
    for (uiIndex = 0; uiIndex < impl->tAdsSymbolUploadInfo.nSymbols; uiIndex++) {
      
      string symbolName = PADSSYMBOLNAME(pAdsSymbolEntry);
      crow::json::wvalue& symbol = impl->findInfo(symbolName);

      pAdsSymbolEntry = populuteSymbolInfo(symbol, symbolName, pAdsSymbolEntry);   
    }

  // Release the allocated memory
  if (pchSymbols)
    delete (pchSymbols);
}

void adsdatasrc::getDatatypeInfo() {
  AdsSymbolUploadInfo2 info;
  long nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOADINFO2, 0, sizeof(info), &info);
  if ( nResult == ADSERR_NOERR ){
      // size of symbol information 
      PBYTE pSym = new BYTE[info.nSymSize];
      if ( pSym ){
          // upload symbols (instances)
          nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOAD, 0, info.nSymSize, pSym);
          // create class-object for each datatype-description
          if ( nResult == ADSERR_NOERR ){
              ParseSymbols(pSym, info.nSymSize);   
          }
      }   
      // get size of datatype description
      PBYTE pDT = new BYTE[info.nDatatypeSize];
      if ( pDT ){
        // upload datatye-descriptions
        nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_DT_UPLOAD, 0, info.nDatatypeSize, pDT);
        if ( nResult == ADSERR_NOERR ){
              ParseDatatypes(pDT, info.nDatatypeSize);   
        }
      }

      impl->parsedSymbols = new CAdsParseSymbols(pSym, info.nSymSize, pDT, info.nDatatypeSize);

      delete [] pSym;
      delete [] pDT;
      /*
      UINT count = impl->parsedSymbols->DatatypeCount();
      for(int i =0; i < count; i++){
        PAdsDatatypeEntry pAdsDatatypeEntry = impl->parsedSymbols->GetTypeByIndex( i );
        string typeName = PADSDATATYPENAME(pAdsDatatypeEntry);
        dataType_impl &dt = impl->getType(typeName);
        if(dt.valid){
          continue;
        }
        dt.valid = true;
        dt.name = typeName;
        dt.members.clear();

        for(int j = 0; j < impl->parsedSymbols->SubSymbolCount(pAdsDatatypeEntry); j++){
          CAdsSymbolInfo info;
          impl->parsedSymbols->SubSymbolInfo(pAdsDatatypeEntry, j, info);
          dt.members.emplace_back(info.name, info.type, info.offs, info.size);
        }
      }
      */

  }
}
/*
void adsdatasrc::getSymbolInfo2( std::string symbolName){

  //Check to see if the symbol exists
  crow::json::wvalue &symbol = impl->findInfo( symbolName );
  if (symbol.keys().size() == 0) {
    BYTE buffer[0xFFFF];
    long nErr = AdsSyncReadWriteReq(
        impl->pAddr, ADSIGRP_SYM_INFOBYNAMEEX, 0, sizeof(buffer), buffer,
        symbolName.length() + 1, (void*)symbolName.c_str());
    if (nErr) {
      cerr << "Error: AdsSyncReadReq: " << nErr << '\n';
    } else {
      PAdsSymbolEntry pAdsSymbolEntry = (PAdsSymbolEntry)buffer;
      populuteSymbolInfo(symbol, symbolName, pAdsSymbolEntry);      
      int nElements = pAdsSymbolEntry->size/sizeof(unsigned long);
      unsigned long *pVal = new unsigned long[nElements];
      AdsSyncReadReq(impl->pAddr, pAdsSymbolEntry->iGroup, pAdsSymbolEntry->iOffs, pAdsSymbolEntry->size, pVal);
      cout << "Value: " << pVal << '\n';; 
    }
  }
  cout << "Symbol: " << symbolName << '\n';
  cout <<  "info: " << impl->findInfo( symbolName ).dump() << '\n';
  cout <<  "value: " << impl->findValue( symbolName ).dump() << '\n';

}
*/

void adsdatasrc::getSymbolInfo( std::string symbolName){

  CAdsSymbolInfo Entry;
  //Check to see if the symbol exists
  impl->parsedSymbols->Symbol( symbolName, Entry );

  impl->getMemberInfo( Entry );    
}


void adsdatasrc::readSymbolValue( std::string symbolName ) {

  crow::json::wvalue info = impl->findValue(symbolName);
  if (info.keys().size() == 0) {  
    getSymbolInfo(symbolName);
    std::cout <<  impl->findValue(symbolName).dump();
  }
  else{
    std::cout << info.dump();
  }
}

void adsdatasrc::ParseSymbols(void* pSymbols, unsigned int nSymSize ){
  impl->m_pSymbols = new BYTE[nSymSize+sizeof(ULONG)];
  if ( impl->m_pSymbols )
  {
    unsigned int m_nSymSize = nSymSize;
    if ( nSymSize )
      memcpy(impl->m_pSymbols, pSymbols, nSymSize);
    *(PULONG)&impl->m_pSymbols[nSymSize] = 0;
    unsigned int m_nSymbols = 0;
    UINT offs = 0;
    while (*(PULONG)&impl->m_pSymbols[offs])
    {
      m_nSymbols++;
      offs += *(PULONG)&impl->m_pSymbols[offs];
    }
//    ASSERT(offs==nSymSize);
    impl->m_ppSymbolArray = new PAdsSymbolEntry[m_nSymbols];
    m_nSymbols = offs = 0;
    while (*(PULONG)&impl->m_pSymbols[offs])
    {
      impl->m_ppSymbolArray[m_nSymbols++] = (PAdsSymbolEntry)&impl->m_pSymbols[offs];
      offs += *(PULONG)&impl->m_pSymbols[offs];
    }
//    ASSERT(offs==nSymSize);
  }
}

void adsdatasrc::ParseDatatypes(void* pDatatypes, unsigned int nDTSize){
	impl->m_pDatatypes = new BYTE[nDTSize+sizeof(ULONG)];
	if ( impl->m_pDatatypes )
	{
		unsigned int m_nDTSize = nDTSize;
		if ( nDTSize )
			memcpy(impl->m_pDatatypes, pDatatypes, nDTSize);
		*(PULONG)&impl->m_pDatatypes[nDTSize] = 0;
		unsigned int m_nDatatypes = 0;
		UINT offs = 0;
		while (*(PULONG)&impl->m_pDatatypes[offs])
		{
			m_nDatatypes++;
			offs += *(PULONG)&impl->m_pDatatypes[offs];
		}
//		ASSERT(offs==nDTSize);
		impl->m_ppDatatypeArray = new PAdsDatatypeEntry[m_nDatatypes];
		m_nDatatypes = offs = 0;
		while (*(PULONG)&impl->m_pDatatypes[offs])
		{
			impl->m_ppDatatypeArray[m_nDatatypes++] = (PAdsDatatypeEntry)&impl->m_pDatatypes[offs];
			offs += *(PULONG)&impl->m_pDatatypes[offs];
		}
//		ASSERT(offs==nDTSize);
	}
}

crow::json::wvalue adsdatasrc::getVariable(std::string symbolName){
  return impl->findValue(symbolName);
}