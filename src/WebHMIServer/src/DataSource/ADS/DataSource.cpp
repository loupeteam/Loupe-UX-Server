#include "DataSource.h"
#include "DataSource_impl.h"

#include <iostream>

#define impl ((adsdatasrc_impl*)_impl)

using namespace std;

adsdatasrc_impl* static_impl;

adsdatasrc::adsdatasrc()
{
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

adsdatasrc::~adsdatasrc()
{
    impl->nErr = AdsPortClose();
    if (impl->nErr) {
        cerr << "Error: AdsPortClose: " << impl->nErr << '\n';
    }
    delete impl;
}

void adsdatasrc::readPlcData()
{
    while (impl->readInfo() != 0) {
        Sleep(1000);
    }
    impl->cacheDataTypes();
}

bool adsdatasrc::ready() { return impl->ready; }

void adsdatasrc::readSymbolValue(std::string symbolName)
{
    if (!impl->ready) {
        return;
    }

    symbolMetadata info = impl->findInfo(symbolName);
    unsigned long size = info.size;
    BYTE* buffer = new BYTE[size];

    // Read a variable from ADS
    long nResult =
        AdsSyncReadReq(impl->pAddr, info.group, info.offset, size, buffer);

    if (nResult == ADSERR_NOERR) {
        crow::json::wvalue& var = impl->findValue(symbolName);
        impl->parseBuffer(var, info.type, buffer, size);
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
}

void adsdatasrc::readSymbolValue(std::vector<std::string> symbolNames)
{
    if (!ready()) {
        return;
    }
    unsigned long size = 0;
    long reqNum = symbolNames.size();
    dataPar* parReq = new dataPar[symbolNames.size()];
    dataPar* parReqPop = parReq;
    for (auto symbolName : symbolNames) {
        symbolMetadata info = impl->findInfo(symbolName);
        parReqPop->indexGroup = info.group;
        parReqPop->indexOffset = info.gOffset;
        parReqPop->length = info.size;
        parReqPop++;
        size += info.size;
    }

    BYTE* buffer = new BYTE[size + 4 * reqNum];

    // Measure time for ADS-Read
    auto start = std::chrono::high_resolution_clock::now();

    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(
        impl->pAddr,
        0xf080, // Sum-Command, response will contain ADS-error code for each
                // ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        4 * reqNum + size, // we request additional "error"-flag(long) for each
                           // ADS-sub commands
        buffer,          // pointer to buffer for ADS-data
        12 * reqNum, // send 12 bytes (3 * long : IG, IO, Len) of each ADS-sub
                     // command
        parReq);   // pointer to buffer for ADS-commands

    auto finish = std::chrono::high_resolution_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
    std::cout << "ADS-Read: " << elapsed.count() / 1000.0 << " ms\n"
              << std::flush;

    start = std::chrono::high_resolution_clock::now();

    if (nResult == ADSERR_NOERR) {
        PBYTE pObjAdsRes = (BYTE*)buffer + (reqNum * 4); // point to ADS-data
        PBYTE pObjAdsErrRes = (BYTE*)buffer;          // point to ADS-err
        PBYTE pdata = pObjAdsRes;
        for (auto symbolName : symbolNames) {
            long result = *(long*)pObjAdsErrRes;
            pObjAdsErrRes += 4;
            if (result != ADSERR_NOERR) {
                cerr << "Error: AdsSyncReadReq: " << result << '\n';
                continue;
            }
            crow::json::wvalue& var = impl->findValue(symbolName);
            symbolMetadata& info = impl->findInfo(symbolName);
            if (info.valid == false) {
                impl->cacheSymbolInfo(symbolName);
            }

            impl->parseBuffer(var, info, pdata, info.size);

            pdata += info.size;
        }
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    finish = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
    std::cout << "Parse: " << elapsed.count() / 1000.0 << " ms\n"
              << std::flush;

    delete[] buffer;
    delete[] parReq;
}

void adsdatasrc::writeSymbolValue(std::string symbolName)
{
    if (!impl->ready) {
        return;
    }

    symbolMetadata info = impl->findInfo(symbolName);
    unsigned long size = info.size;
    BYTE* buffer = new BYTE[size];

    // Read a variable from ADS
    long nResult = AdsSyncReadReq(impl->pAddr, info.group, info.offset, size, buffer);

    if (nResult == ADSERR_NOERR) {
        crow::json::wvalue& var = impl->findValue(symbolName);
        impl->parseBuffer(var,
                          info.type,
                          buffer,
                          size);
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
}

void adsdatasrc::gatherBaseTypeNames_Member(crow::json::rvalue& member, std::string prefix,
                                            std::vector<pair<std::string, std::string> >& names)
{
    if (member.t() == crow::json::type::Object) {
        gatherBaseTypeNames(member, prefix + ".", names);
    } else if (member.t() == crow::json::type::List) {
        gatherBaseTypeNames(member, prefix, names);
    } else {
        symbolMetadata info = impl->findInfo(prefix);
        if (info.flags_struct.DATATYPE) {
            cout << "Can't write datatype: " << prefix << ", Skipping" << endl;
        } else if (info.flags_struct.PROPITEM) {
            cout << "Can't write property: " << prefix << ", Skipping" << endl;
        } else if (info.flags_struct.METHODDEREF) {
            cout << "Can't write METHODDEREF: " << prefix << ", Skipping" << endl;
        } else if (info.size == 0) {
            cout << "Can't write size 0: " << prefix << ", Skipping" << endl;
        } else if ((info.size > 8) && (info.dataType == 65)) {
            cout << "Can't write single structure greater than 8 with no members: " << prefix << ", Skipping" << endl;
        } else {
            names.emplace_back(make_pair(prefix, std::string(member)));
        }
    }
}
//Go through the structure and gather all the names of the members that are base types
void adsdatasrc::gatherBaseTypeNames(crow::json::rvalue&              packet,
                                     std::string&                     prefix,
                                     std::vector<pair<std::string,
                                                      std::string> >& names)
{
    //Go through the members of the packet
    //If the member is a base type, add it to the list
    //If the member is a structure, call this function on it
    if (packet.t() == crow::json::type::Object) {
        for ( auto key : packet.keys()) {
            crow::json::rvalue member = packet[key];
            gatherBaseTypeNames_Member(member, prefix + key, names);
        }
    } else if (packet.t() == crow::json::type::List) {
        for ( int i = 0; i < packet.size(); i++ ) {
            crow::json::rvalue member = packet[i];
            gatherBaseTypeNames_Member(member, prefix + "[" + std::to_string(i) + "]", names);
        }
    }
}

void adsdatasrc::writeSymbolValue(crow::json::rvalue packet)
{
    if (!ready()) {
        return;
    }

    std::vector<pair<std::string, std::string> > symbolNames;
    gatherBaseTypeNames(packet, std::string(""), symbolNames);

    unsigned long size = 0;
    long reqNum = 0;
    //Go through and figure out how much space and how many requests we need
    for (auto symbolName : symbolNames) {
        symbolMetadata& info = impl->findInfo(symbolName.first);
        if (info.valid == false) {
            impl->cacheSymbolInfo(symbolName.first);
        }
        if (info.valid == true) {
            reqNum++;
            size += info.size;
        } else {
            cerr << "Error: Could not find symbol " << symbolName.first << '\n';
        }
    }

    //Allocate memory for the requests
    //The request starts with dataPar structures, followed by the data all in the same buffer
    BYTE* buffer = new BYTE[reqNum * sizeof(dataPar) + size];
    dataPar* parReqPop = (dataPar*)buffer;
    BYTE* pdata = buffer + (reqNum * sizeof(dataPar));
    for (auto symbolName : symbolNames) {
        if (impl->encodeBuffer(symbolName.first, pdata, symbolName.second, size)) {
            symbolMetadata info = impl->findInfo(symbolName.first);
            parReqPop->indexGroup = info.group;
            parReqPop->indexOffset = info.gOffset;
            parReqPop->length = info.size;
            pdata += info.size;
            parReqPop++;
        } else {
            cerr << "Error: Could not encode value for " << symbolName.first << '\n';
        }
        // symbolMetadata info = impl->findInfo(symbolName.first);
        // if (info.valid == true) {
        //     if (dataType_member_base::encode(info.dataType, pdata, symbolName.second, info.size)) {
        //         parReqPop->indexGroup = info.group;
        //         parReqPop->indexOffset = info.gOffset;
        //         parReqPop->length = info.size;
        //         pdata += info.size;
        //         parReqPop++;
        //     } else {
        //         cerr << "Error: Could not encode value for " << symbolName.first << '\n';
        //     }
        // }
    }

    // Measure time for ADS-Read
    auto start = std::chrono::high_resolution_clock::now();
    PBYTE mAdsSumBufferRes = new BYTE[4 * reqNum];

    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(
        impl->pAddr,
        0xf081, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        4 * reqNum, // we request additional "error"-flag(long) for each ADS-sub commands
        mAdsSumBufferRes,          // pointer to buffer for ADS-data
        reqNum * sizeof(dataPar) + size,      // send x bytes (3 * long : IG, IO, Len) + data
        // command
        buffer);   // pointer to buffer for ADS-commands

    auto finish = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
    std::cout << "ADS-write: " << elapsed.count() / 1000.0 << " ms\n"
              << std::flush;

    if (nResult == ADSERR_NOERR) {
        cout << "success?";
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
    delete mAdsSumBufferRes;
}

crow::json::wvalue adsdatasrc::getSymbolValue(std::string symbolName)
{
    crow::json::wvalue value = impl->findValue(symbolName);
    symbolMetadata& info = impl->findInfo(symbolName);

    if (info.valid == false) {
        impl->cacheSymbolInfo(symbolName);
    }
    return value;
}
