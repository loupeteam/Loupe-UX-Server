#include "adsdatasrc_impl.h"

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

void adsdatasrc::readPlcData(){
  impl->readInfo();
  impl->cacheDataTypes();
}

bool adsdatasrc::ready(){
  return impl->ready;
}
void adsdatasrc::readSymbolValue( std::vector<std::string> symbolNames ){
    if (!ready()) {
      return;
    }
    unsigned long size = 0;
    long reqNum = symbolNames.size();
    dataPar *parReq = new dataPar[symbolNames.size()];
    dataPar *parReqPop = parReq;
    for( auto symbolName : symbolNames){
      symbolMetadata info = impl->findInfo(symbolName);
      parReqPop->indexGroup = info.group;
      parReqPop->indexOffset = info.gOffset;
      parReqPop->length = info.size;
      parReqPop++;
      size += info.size;
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
        
        long result = *(long*)pObjAdsErrRes;
        pObjAdsErrRes += 4;
        if( result != ADSERR_NOERR){
          cerr << "Error: AdsSyncReadReq: " << result << '\n';
          continue;
        }        
        crow::json::wvalue &var = impl->findValue(symbolName);
        symbolMetadata &info = impl->findInfo(symbolName);
        if( info.valid == false) {
          impl->cacheSymbolInfo(symbolName); 
        }

        impl->parseBuffer(var, info.type, pdata, info.size);                                
        pdata += info.size;
      }
    } else {
      cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
    delete[] parReq;
  }
  
void adsdatasrc::readSymbolValue(std::string symbolName) {
  if (!impl->ready) {
    return;
  }

  symbolMetadata info = impl->findInfo(symbolName);
  unsigned long size = info.size;
  BYTE *buffer = new BYTE[size];

  // Read a variable from ADS
  long nResult = AdsSyncReadReq(impl->pAddr, info.group,
                                info.offset, size, buffer);

  if (nResult == ADSERR_NOERR) {
    crow::json::wvalue &var = impl->findValue(symbolName);
    impl->parseBuffer(var, info.type, buffer, size);                                
  } else {
    cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
  }

  delete[] buffer;
}

crow::json::wvalue adsdatasrc::getSymbolValue(std::string symbolName) {

  crow::json::wvalue value = impl->findValue(symbolName);
  symbolMetadata &info = impl->findInfo(symbolName);

  if( info.valid == false) {
    impl->cacheSymbolInfo(symbolName); 
  }
  return value;
}