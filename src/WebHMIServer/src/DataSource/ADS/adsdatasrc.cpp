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
      PAdsDatatypeEntry pAdsDatatypeEntry =
          impl->parsedSymbols->GetTypeByIndex(i);
      string typeName = PADSDATATYPENAME(pAdsDatatypeEntry);
      dataType &dt = impl->getType(typeName);
      if (dt.valid) {
        continue;
      }
      dt.valid = true;
      dt.name = typeName;
      dt.members.clear();

      for (int j = 0;
           j < impl->parsedSymbols->SubSymbolCount(pAdsDatatypeEntry); j++) {
        CAdsSymbolInfo info;
        impl->parsedSymbols->SubSymbolInfo(pAdsDatatypeEntry, j, info);

        if(info.type == "BOOL" || info.type == "BIT" || info.type == "BIT8"){
          dt.members.push_back(new dataType_member_base_typed<bool>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "STRING" || info.type == "WSTRING" ){
          dt.members.push_back(new dataType_member_string{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "WORD"){
          dt.members.push_back(new dataType_member_base_typed<WORD>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "DWORD"){
          dt.members.push_back(new dataType_member_base_typed<DWORD>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "BYTE"){
          dt.members.push_back(new dataType_member_base_typed<BYTE>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "INT"){
          dt.members.push_back(new dataType_member_base_typed<INT>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "DINT"){
          dt.members.push_back(new dataType_member_base_typed<long int>{info.name, info.type, info.iOffs, info.size});
        }

        else if(info.type == "FLOAT" || info.type == "REAL"){
          dt.members.push_back(new dataType_member_base_typed<float>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "LREAL" || info.type == "DOUBLE"){
          dt.members.push_back(new dataType_member_base_typed<double>{info.name, info.type, info.iOffs, info.size});
        }
        else if(info.type == "POINTER TO STRING(80)"){
          dt.members.push_back(new dataType_member_unsupported{info.name, info.type, info.iOffs, info.size});
        }
        else{
          cout << "Unknown type: " << info.type << '\n' <<std::flush;
          dt.members.push_back(new datatype_member{info.name, info.type, info.iOffs, info.size});
        }
      }
    }
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
      AdsSyncReadReq(impl->pAddr, pAdsSymbolEntry->iGroup,
pAdsSymbolEntry->iOffs, pAdsSymbolEntry->size, pVal); cout << "Value: " << pVal
<< '\n';;
    }
  }
  cout << "Symbol: " << symbolName << '\n';
  cout <<  "info: " << impl->findInfo( symbolName ).dump() << '\n';
  cout <<  "value: " << impl->findValue( symbolName ).dump() << '\n';

}
*/

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

  dataType info = impl->getType( datatype );
  //Go through all the members and parse them
  for( dataType_member_base *member : info.members){
    dataType  &memberInfo =  impl->getType( member->type );
    if( memberInfo.valid ){
      if( !member->parse( variable, buffer + member->offset, member->size ) ){
        parseBuffer( variable[member->name], member->type, buffer + member->offset, member->size );
      }
    }
  }
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
  byte *buffer = new byte[size];


  // Read a variable from ADS
  long nResult = AdsSyncReadReq(impl->pAddr, group.get((int64_t)0),
                                offset.get((int64_t)0), size, buffer);

  crow::json::wvalue &var = impl->findValue(symbolName);
  parseBuffer(var, type.get(string("")), buffer, size);                                

  if (nResult == ADSERR_NOERR) {
//    cout << "Value: " << info.dump() << '\n';
  } else {
    cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
  }

  delete[] buffer;

  // crow::json::wvalue info = impl->findValue(symbolName);
  // if (info.keys().size() == 0) {
  //   getSymbolInfo(symbolName);
  //   std::cout << impl->findValue(symbolName).dump();
  // } else {
  //   std::cout << info.dump();
  // }
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

  readSymbolValue(symbolName);

  crow::json::wvalue value = impl->findValue(symbolName);
  if (value.keys().size() == 0) {
    getSymbolInfo(symbolName);
    return impl->findValue(symbolName);
  } else {
    return value;
  }
}