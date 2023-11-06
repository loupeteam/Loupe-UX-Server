#include "DataSource.h"
#include "DataSource_impl.h"

#include <iostream>

#define impl ((adsdatasrc_impl*)_impl)

using namespace std;

adsdatasrc_impl* static_impl;

adsdatasrc::adsdatasrc()
{
    _impl = new adsdatasrc_impl();
    //TODO: Make this configurable
    impl->nPort = AdsPortOpen();

    // set default communication parameters
    NetIDType netId = {192, 168, 0, 1, 1, 1};
    this->setPlcCommunicationParameters(netId, 851);

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


void adsdatasrc::setPlcCommunicationParameters(NetIDType netId, uint16_t port)
{

    for (int i = 0; i < 6; i++) {
        impl->Addr.netId.b[i] = netId.b[i];
    }
    
    impl->pAddr->port = port;
}


void adsdatasrc::readPlcData()
{
    cout << "Attempting to connect to NetID " << \
            (int)impl->Addr.netId.b[0] << '.' << \
            (int)impl->Addr.netId.b[1] << '.' << \
            (int)impl->Addr.netId.b[2] << '.' << \
            (int)impl->Addr.netId.b[3] << '.' << \
            (int)impl->Addr.netId.b[4] << '.' << \
            (int)impl->Addr.netId.b[5] << \
            " on Port " << impl->pAddr->port << " ..." << '\n';

    while (impl->readInfo() != 0) {
        Sleep(1000);
        cerr << "No PLC Connections, will try again " << '\n';
    }
}

bool adsdatasrc::ready() { return impl->ready; }

void adsdatasrc::readSymbolValue(std::string symbolName)
{
    if (!ready()) {
        return;
    }

    //Get the symbol info
    symbolMetadata info = impl->findInfo(symbolName);

    //Allocate a buffer for the data
    unsigned long size = info.size;
    BYTE* buffer = new BYTE[size];

    // Read a variable from ADS
    long nResult = AdsSyncReadReq(impl->pAddr, info.group, info.offset, size, buffer);

    //Parse the buffer into the variable
    if (nResult == ADSERR_NOERR) {
        crow::json::wvalue& var = impl->findValue(symbolName);
        impl->parseBuffer(var, info, buffer, size);
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

    //Allocate a buffer for the data
    long reqNum = symbolNames.size();
    dataPar* parReq = new dataPar[symbolNames.size()];
    dataPar* parReqPop = parReq;

    //Go through and figure out how much space and how many requests we need
    unsigned long size = 0;
    for (auto symbolName : symbolNames) {
        symbolMetadata info = impl->findInfo(symbolName);
        parReqPop->indexGroup = info.group;
        parReqPop->indexOffset = info.gOffset;
        parReqPop->length = info.size;
        parReqPop++;
        size += info.size;
    }

    //Allocate a buffer for the request and data
    BYTE* buffer = new BYTE[size + 4 * reqNum];

    // Measure time for ADS-Read
    auto start = getTimestamp();

    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(
        impl->pAddr,
        ADSIGRP_SUMUP_READ, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        4 * reqNum + size, // we request additional "error"-flag(long) for each ADS-sub commands
        buffer,          // pointer to buffer for ADS-data
        12 * reqNum, // send 12 bytes (3 * long : IG, IO, Len) of each ADS-sub command
        parReq);   // pointer to buffer for ADS-commands

    measureTime("ADS-Read: ", start);

    start = getTimestamp();

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
            impl->parseBuffer(var, info, pdata, info.size);
            pdata += info.size;
        }
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    measureTime("Parse: ", start);

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
                          info,
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
        for ( size_t i = 0; i < packet.size(); i++ ) {
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
    }

    // Measure time for ADS-Read
    auto start = getTimestamp();
    PBYTE mAdsSumBufferRes = new BYTE[4 * reqNum];

    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(
        impl->pAddr,
        ADSIGRP_SUMUP_WRITE, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        4 * reqNum, // we request additional "error"-flag(long) for each ADS-sub commands
        mAdsSumBufferRes,          // pointer to buffer for ADS-data
        reqNum * sizeof(dataPar) + size,      // send x bytes (3 * long : IG, IO, Len) + data command
        buffer);   // pointer to buffer for ADS-commands

    measureTime("ADS-write: ", start);

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

    return value;
}
