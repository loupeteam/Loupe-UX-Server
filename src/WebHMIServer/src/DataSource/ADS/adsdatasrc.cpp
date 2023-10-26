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

void adsdatasrc::readSymbolValue( std::vector<std::string> symbolNames ){
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
          impl->cacheSymbolInfo(symbolName); 
          info["valid"] = true;
        }

        impl->parseBuffer(var, type.get(string("")), pdata, sizereader.get((int64_t)0));                                
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
    impl->parseBuffer(var, type.get(string("")), buffer, size);                                
  } else {
    cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
  }

  delete[] buffer;
}

crow::json::wvalue adsdatasrc::getSymbolValue(std::string symbolName) {

  crow::json::wvalue value = impl->findValue(symbolName);
  crow::json::wvalue &info = impl->findInfo(symbolName);
  crow::json::wvalue_reader valid{ref(info["valid"])};
  if( valid.get(false) == false) {

    impl->cacheSymbolInfo(symbolName); 
    info["valid"] = true;
  }
  return value;
}