#include "./adsdatasrc.h"
#include "./adsdatasrc_impl.h"

#include "adsdatasrc.h"
#include <iostream>

#define impl ((adsdatasrc_impl *)_impl)

using namespace std;

adsdatasrc_impl *static_impl;

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

void adsdatasrc::getGlobalSymbolInfo() {
  char *pchSymbols = NULL;

  impl->nErr = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOADINFO, 0x0,
                              sizeof(impl->tAdsSymbolUploadInfo),
                              &impl->tAdsSymbolUploadInfo);
  if (impl->nErr) {
    cerr << "Error: AdsSyncReadReq: " << impl->nErr << '\n';
    return;
  }

  pchSymbols = new char[impl->tAdsSymbolUploadInfo.nSymSize];
  // assert(pchSymbols);
  long nErr;

  // Read information of the PLC-variable
  nErr = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOAD, 0,
                        impl->tAdsSymbolUploadInfo.nSymSize, pchSymbols);

  if (nErr)
    cerr << "Error: AdsSyncReadReq: " << nErr << '\n';

  // Print out the information of the PLC-variable
  PAdsSymbolEntry pAdsSymbolEntry = (PAdsSymbolEntry)pchSymbols;

  int uiIndex;
  for (uiIndex = 0; uiIndex < impl->tAdsSymbolUploadInfo.nSymbols; uiIndex++) {

    string symbolName = PADSSYMBOLNAME(pAdsSymbolEntry);
    crow::json::wvalue &symbol = impl->findInfo(symbolName);

    pAdsSymbolEntry = populateSymbolInfo(symbol, symbolName, pAdsSymbolEntry);
  }

  // Release the allocated memory
  if (pchSymbols)
    delete (pchSymbols);

  ready = true;
}

void adsdatasrc::getDatatypeInfo() {
  AdsSymbolUploadInfo2 info;
  long nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOADINFO2, 0,
                                sizeof(info), &info);
  unordered_map<string, bool> warns;
  if (nResult == ADSERR_NOERR) {
    // size of symbol information
    PBYTE pSym = new BYTE[info.nSymSize];
    if (pSym) {
      // upload symbols (instances)
      nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_UPLOAD, 0,
                               info.nSymSize, pSym);
      // create class-object for each datatype-description
      if (nResult == ADSERR_NOERR) {
        ParseSymbols(pSym, info.nSymSize);
      }
    }
    // get size of datatype description
    PBYTE pDT = new BYTE[info.nDatatypeSize];
    if (pDT) {
      // upload datatye-descriptions
      nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_DT_UPLOAD, 0,
                               info.nDatatypeSize, pDT);
      if (nResult == ADSERR_NOERR) {
        ParseDatatypes(pDT, info.nDatatypeSize);
      }
    }

    impl->parsedSymbols =
        new CAdsParseSymbols(pSym, info.nSymSize, pDT, info.nDatatypeSize);

    delete[] pSym;
    delete[] pDT;
    UINT count = impl->parsedSymbols->DatatypeCount();
    for (int i = 0; i < count; i++) {
      PAdsDatatypeEntry pAdsDatatypeEntry = impl->parsedSymbols->GetTypeByIndex(i);
      string typeName = PADSDATATYPENAME(pAdsDatatypeEntry);
      dataType_member_base *dt = impl->getType(typeName);
      if (dt->valid) {
        continue;
      }
      dt->name = typeName;
    }
  }
}

void adsdatasrc::getSymbolInfo(std::string symbolName) {

  if (!ready) {
    return;
  }

  CAdsSymbolInfo Entry;

  // Check to see if the symbol exists
  impl->parsedSymbols->Symbol(symbolName, Entry);

  impl->getMemberInfo(Entry);
}

void adsdatasrc::parseBuffer(crow::json::wvalue &variable, string &datatype, void *gbuffer, unsigned long size ){
  
  byte *buffer = (byte*)gbuffer;

  dataType_member_base* info = impl->getType( datatype );
  
  if(!info->valid){
    info->valid = true;
    impl->PopulateChildren(info);
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


void adsdatasrc::updateVariables( std::vector<std::string> symbolNames ){
    if (!ready) {
      return;
    }
    unsigned long size = 0;
    long reqNum = symbolNames.size();
    dataPar *parReq = new dataPar[symbolNames.size()];
    dataPar *parReqPop = parReq;
    for( auto symbolName : symbolNames){
      crow::json::wvalue info = impl->findInfo(symbolName);
      crow::json::wvalue_reader group{ref(info["group"])};
      crow::json::wvalue_reader offset{ref(info["offset"])};
      crow::json::wvalue_reader sizereader{ref(info["size"])};
      crow::json::wvalue_reader type{ref(info["type"])};
      parReqPop->indexGroup = group.get((int64_t)0);
      parReqPop->indexOffset = offset.get((int64_t)0);
      parReqPop->length = sizereader.get((int64_t)0);
      parReqPop++;
      size += sizereader.get((int64_t)0);
    }

    BYTE *buffer = new BYTE[size + 4*reqNum];

    //Measure time for ADS-Read
    auto start = std::chrono::high_resolution_clock::now();

    
    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(impl->pAddr, 
                                    0xf080, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
                                    reqNum, // Number of ADS-Sub-commands
                                    4*reqNum + size, // we request additional "error"-flag(long) for each ADS-sub commands
                                    buffer, // pointer to buffer for ADS-data
                                    12*reqNum, // send 12 bytes (3 * long : IG, IO, Len) of each ADS-sub command
                                    parReq); // pointer to buffer for ADS-commands

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    std::cout << "ADS-Read: " << elapsed.count() << " s\n" << std::flush;


    if (nResult == ADSERR_NOERR) {
      PBYTE pObjAdsRes = (BYTE*)buffer + (reqNum*4);	// point to ADS-data
      PBYTE pObjAdsErrRes = (BYTE*)buffer;				// point to ADS-err
      PBYTE pdata = pObjAdsRes;
      for( auto symbolName : symbolNames){

        crow::json::wvalue &var = impl->findValue(symbolName);
        crow::json::wvalue &info = impl->findInfo(symbolName);
        crow::json::wvalue_reader sizereader{ref(info["size"])};
        crow::json::wvalue_reader type{ref(info["type"])};
        crow::json::wvalue_reader valid{ref(info["valid"])};
        if( valid.get(false) == false) {
          getSymbolInfo(symbolName); 
          info["valid"] = true;
        }

        parseBuffer(var, type.get(string("")), pdata, sizereader.get((int64_t)0));                                
        pdata += sizereader.get((int64_t)0);
      }
    } else {
      cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
    delete[] parReq;
  }
  
void adsdatasrc::readSymbolValue(std::string symbolName) {
  if (!ready) {
    return;
  }

  crow::json::wvalue info = impl->findInfo(symbolName);
  crow::json::wvalue_reader group{ref(info["group"])};
  crow::json::wvalue_reader offset{ref(info["offset"])};
  crow::json::wvalue_reader sizereader{ref(info["size"])};
  crow::json::wvalue_reader type{ref(info["type"])};
  unsigned long size = sizereader.get((int64_t)0);
  BYTE *buffer = new BYTE[size];

  // Read a variable from ADS
  long nResult = AdsSyncReadReq(impl->pAddr, group.get((int64_t)0),
                                offset.get((int64_t)0), size, buffer);

  if (nResult == ADSERR_NOERR) {
    crow::json::wvalue &var = impl->findValue(symbolName);
    parseBuffer(var, type.get(string("")), buffer, size);                                
  } else {
    cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
  }

  delete[] buffer;
}

void adsdatasrc::ParseSymbols(void *pSymbols, unsigned int nSymSize) {
  impl->m_pSymbols = new BYTE[nSymSize + sizeof(ULONG)];
  if (impl->m_pSymbols) {
    unsigned int m_nSymSize = nSymSize;
    if (nSymSize)
      memcpy(impl->m_pSymbols, pSymbols, nSymSize);
    *(PULONG)&impl->m_pSymbols[nSymSize] = 0;
    unsigned int m_nSymbols = 0;
    UINT offs = 0;
    while (*(PULONG)&impl->m_pSymbols[offs]) {
      m_nSymbols++;
      offs += *(PULONG)&impl->m_pSymbols[offs];
    }
    //    ASSERT(offs==nSymSize);
    impl->m_ppSymbolArray = new PAdsSymbolEntry[m_nSymbols];
    m_nSymbols = offs = 0;
    while (*(PULONG)&impl->m_pSymbols[offs]) {
      impl->m_ppSymbolArray[m_nSymbols++] =
          (PAdsSymbolEntry)&impl->m_pSymbols[offs];
      offs += *(PULONG)&impl->m_pSymbols[offs];
    }
    //    ASSERT(offs==nSymSize);
  }
}

void adsdatasrc::ParseDatatypes(void *pDatatypes, unsigned int nDTSize) {
  impl->m_pDatatypes = new BYTE[nDTSize + sizeof(ULONG)];
  if (impl->m_pDatatypes) {
    unsigned int m_nDTSize = nDTSize;
    if (nDTSize)
      memcpy(impl->m_pDatatypes, pDatatypes, nDTSize);
    *(PULONG)&impl->m_pDatatypes[nDTSize] = 0;
    unsigned int m_nDatatypes = 0;
    UINT offs = 0;
    while (*(PULONG)&impl->m_pDatatypes[offs]) {
      m_nDatatypes++;
      offs += *(PULONG)&impl->m_pDatatypes[offs];
    }
    //		ASSERT(offs==nDTSize);
    impl->m_ppDatatypeArray = new PAdsDatatypeEntry[m_nDatatypes];
    m_nDatatypes = offs = 0;
    while (*(PULONG)&impl->m_pDatatypes[offs]) {
      impl->m_ppDatatypeArray[m_nDatatypes++] =
          (PAdsDatatypeEntry)&impl->m_pDatatypes[offs];
      offs += *(PULONG)&impl->m_pDatatypes[offs];
    }
    //		ASSERT(offs==nDTSize);
  }
}

crow::json::wvalue adsdatasrc::getVariable(std::string symbolName) {

  crow::json::wvalue value = impl->findValue(symbolName);
  crow::json::wvalue &info = impl->findInfo(symbolName);
  crow::json::wvalue_reader valid{ref(info["valid"])};
  if( valid.get(false) == false) {
    getSymbolInfo(symbolName); 
    info["valid"] = true;
  }
  return value;
}