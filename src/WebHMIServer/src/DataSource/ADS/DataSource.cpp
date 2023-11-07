#include "DataSource.h"
#include "DataSource_impl.h"

#include <iostream>

#define impl ((adsdatasrc_impl*)_impl)

using namespace std;

//Note: This static impl is just to make debugging easier
adsdatasrc_impl* static_impl;

//TODO: prefer not use a single global variable
//  But we need to be able to access the callback from a static function
adsdatasrc* pAdsdatasrc = nullptr;

adsdatasrc::adsdatasrc()
{
    _impl = new adsdatasrc_impl();
    //TODO: Make this configurable
    impl->nPort = AdsPortOpen();
    impl->Addr.netId.b[0] = 192;
    impl->Addr.netId.b[1] = 168;
    impl->Addr.netId.b[2] = 0;
    impl->Addr.netId.b[3] = 46;
    impl->Addr.netId.b[4] = 1;
    impl->Addr.netId.b[5] = 1;

    impl->Addr.port = 851;

//    this->readPlcData();

    //Note: This static impl is just to make debugging easier
    static_impl = impl;
    pAdsdatasrc = this;
}

adsdatasrc::~adsdatasrc()
{
    impl->nErr = AdsPortClose();
    if (impl->nErr) {
        cerr << "Error: AdsPortClose: " << impl->nErr << '\n';
    }
    delete impl;
}

//Read the symbol and datatype data from the PLC
void adsdatasrc::readPlcData()
{
    while (impl->readInfo() != 0) {
        Sleep(1000);
        cerr << "No PLC Connections, will try again " << '\n';
    }
}

void adsdatasrc::connect()
{
    //this->registerDUTChangeCallback();
    this->readPlcData();
}

void __stdcall SymbolChangedCallback(AmsAddr* pAddr, AdsNotificationHeader* pNotification, unsigned long hUser)
{
    pAdsdatasrc->readPlcData();
}

void adsdatasrc::registerDUTChangeCallback()
{
    if (this->adsChangeHandle != 0) {
        return;
    }
    long nErr;
    AdsNotificationAttrib adsNotificationAttrib;

    // Specify attributes of the notification
    adsNotificationAttrib.cbLength = 1;
    adsNotificationAttrib.nTransMode = ADSTRANS_SERVERONCHA;
    adsNotificationAttrib.nMaxDelay = 50000; // 50ms
    adsNotificationAttrib.nCycleTime = 50000; // 50ms
    // Start notification for changes to the symbol table
    nErr = AdsSyncAddDeviceNotificationReq(impl->pAddr,
                                           ADSIGRP_SYM_VERSION,
                                           0,
                                           &adsNotificationAttrib,
                                           SymbolChangedCallback,
                                           0,
                                           &this->adsChangeHandle);
    if (nErr) { cerr << "Error: AdsSyncAddDeviceNotificationReq: " << nErr << '\n';}
}

//Check if we have read the PLC data
bool adsdatasrc::ready() { return impl->ready; }

//Read the value of a single symbol
void adsdatasrc::readSymbolValue(std::string symbolName)
{
    if (!ready()) {
        return;
    }

    //Get the symbol info
    symbolMetadata& info = impl->findInfo(symbolName);

    //If read failed we assume the value can't be read
    if (info.readFail) {
        crow::json::wvalue& var = impl->findValue(symbolName);
        var.clear();
        return;
    }

    //If the symbol is a property and we don't have a handle, get one
    if ((info.flags_struct.PROPITEM == true) && (info.handle == 0)) {
        getSymbolHandle(symbolName);
    }

    //Allocate a buffer for the data
    unsigned long size = info.size;
    BYTE* buffer = new BYTE[size];
    long nResult;
    if (info.flags_struct.PROPITEM) {
        // Read value of a PLC variable (by handle)
        nResult = AdsSyncReadReq(impl->pAddr, ADSIGRP_SYM_VALBYHND, info.handle, size, buffer);
    } else {
        // Read a variable from ADS
        nResult = AdsSyncReadReq(impl->pAddr, info.group, info.offset, size, buffer);
    }

    //Parse the buffer into the variable
    if (nResult == ADSERR_NOERR) {
        info.readFail = false;
        crow::json::wvalue& var = impl->findValue(symbolName);
        impl->parseBuffer(var, info, buffer, size);
    } else {
        crow::json::wvalue& var = impl->findValue(symbolName);

        //This error means that we just couldn't read the address. Probably null
        if (nResult == 1793) {
            var.clear();
        }
        //This error means there isn't a getter for this property
        else if (nResult == 1796) {
            info.readFail = true;
            var.clear();
        }
        //Unknown error, output it for the user
        else {
            var = string("Error ") + to_string(nResult);
            cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
        }
    }

    delete[] buffer;
}

//Read the value of a list of symbols
void adsdatasrc::readSymbolValue(std::vector<std::string> reqSymbolNames)
{
    if (!ready()) {
        return;
    }

    std::vector<std::string> symbolNames;
    //Reserve space for the required symbols and the properties for performance
    symbolNames.reserve(reqSymbolNames.size() + impl->propertyReads.size());

    //Insert all the symbols the user is specifically requesting
    // Note: This is done first so that the properties can override the symbols
    // TODO: We should probably check if the user is requesting a property and
    //       if so, remove the symbol from the list
    // TODO: We should probably check if the user is requesting a property that
    //       is known to throw errors
    symbolNames.insert(symbolNames.end(), reqSymbolNames.begin(), reqSymbolNames.end());

    //Go through the properties and add them to the list if they are within any structures that are requested
    for (auto property : impl->propertyReads) {
        if (std::find(symbolNames.begin(), symbolNames.end(), property) == symbolNames.end()) {
            symbolNames.push_back(property);
        }
    }

    //Allocate a buffer for the data
    long reqNum = symbolNames.size();
    dataPar* parReq = new dataPar[symbolNames.size()];
    dataPar* parReqPop = parReq;

    //Go through and figure out how much space and how many requests we need
    unsigned long size = 0;
    for (auto symbolName : symbolNames) {
        symbolMetadata& info = impl->findInfo(symbolName);
        if (info.flags_struct.PROPITEM == true) {
            if (info.handle == 0) {
                getSymbolHandle(symbolName);
            }
            parReqPop->indexGroup = ADSIGRP_SYM_VALBYHND;
            parReqPop->indexOffset = info.handle;
            parReqPop->length = info.size;
        } else {
            parReqPop->indexGroup = info.group;
            parReqPop->indexOffset = info.gOffset;
            parReqPop->length = info.size;
        }
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

    if (nResult == ADSERR_NOERR) {
        PBYTE pObjAdsRes = (BYTE*)buffer + (reqNum * 4); // point to ADS-data
        PBYTE pObjAdsErrRes = (BYTE*)buffer;          // point to ADS-err
        PBYTE pdata = pObjAdsRes;
        for (auto symbolName : symbolNames) {
            long result = *(long*)pObjAdsErrRes;
            pObjAdsErrRes += 4;
            symbolMetadata& info = impl->findInfo(symbolName);
            crow::json::wvalue& var = impl->findValue(symbolName);
            if (result != ADSERR_NOERR) {
                cerr << "Error: AdsSyncReadReq: " << result << " " << symbolName << '\n';
                if (result == 1793) {}
                //This error means there isn't a getter for this property
                else if (result == 1796) {
                    var.clear();
                    //Remove this symbol from the reads and mark it as a failure
                    info.readFail = true;
                    impl->propertyReads.erase(std::remove(impl->propertyReads.begin(),
                                                          impl->propertyReads.end(),
                                                          symbolName),
                                              impl->propertyReads.end());
                }
                //Increment the pointer to the data even if there was a read failure
                pdata += info.size;
                continue;
            }
            //Parse the buffer into the variable
            impl->parseBuffer(var, info, pdata, info.size);
            pdata += info.size;
        }
    } else {
        cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
    }

    delete[] buffer;
    delete[] parReq;
}

//Write the value of a json packet to the PLC
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
        if ((info.flags_struct.PROPITEM == true) && (info.handle == 0)) {
            getSymbolHandle(symbolName.first);
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
            symbolMetadata& info = impl->findInfo(symbolName.first);

            if (info.handle) {
                parReqPop->indexGroup = ADSIGRP_SYM_VALBYHND;
                parReqPop->indexOffset = info.handle;
                parReqPop->length = info.size;
            } else {
                parReqPop->indexGroup = info.group;
                parReqPop->indexOffset = info.gOffset;
                parReqPop->length = info.size;
            }
            pdata += info.size;
            parReqPop++;
        } else {
            cerr << "Error: Could not encode value for " << symbolName.first << '\n';
        }
    }

    PBYTE mAdsSumBufferRes = new BYTE[reqNum * sizeof(long)];

    // Read a variable from ADS
    long nResult = AdsSyncReadWriteReq(
        impl->pAddr,
        ADSIGRP_SUMUP_WRITE, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        reqNum * sizeof(long), // we request additional "error"-flag(long) for each ADS-sub commands
        mAdsSumBufferRes,          // pointer to buffer for ADS-data
        reqNum * sizeof(dataPar) + size,      // send x bytes (3 * long : IG, IO, Len) + data command
        buffer);   // pointer to buffer for ADS-commands

    if (nResult == ADSERR_NOERR) {
        long* pResult = (long*)mAdsSumBufferRes;
        //Output the individual statuses
        for (auto symbolName : symbolNames) {
            long result = *pResult;
            pResult += 1;
            if (result != ADSERR_NOERR) {
                cerr << "Error Writing: " << result << " " << symbolName.first << '\n';
            }
        }
    } else {
        cerr << "Error: AdsSyncReadWriteReq: " << nResult << '\n';
    }

    delete[] buffer;
    delete mAdsSumBufferRes;
}

//Get a handle to symbols that are not variables
void adsdatasrc::getSymbolHandle(std::string symbolName)
{
    if (!ready()) {
        return;
    }

    //Get the symbol info
    symbolMetadata& info = impl->findInfo(symbolName);

    //If the symbol is a property and we don't have a handle, get one
    if (info.handle == 0) {
        // Fetch handle for an <szVar> PLC variable
        long nResult = AdsSyncReadWriteReq(impl->pAddr,
                                           ADSIGRP_SYM_HNDBYNAME,
                                           0x0,
                                           sizeof(info.handle),
                                           &info.handle,
                                           info.name.size(),
                                           (void*)info.name.c_str());
        //Parse the buffer into the variable
        if (nResult == ADSERR_NOERR) {} else {
            cerr << "Error: AdsSyncReadReq: " << nResult << '\n';
        }
    }
}

//Get a handle to a list of symbols that are not variables
void adsdatasrc::getSymbolHandle(std::vector<std::string> symbolNames)
{
    //TODO: Implement this
    //Is it even possible to get multiple handles at once?
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

//Go through the structure and gather all the names of the members that are base types
void adsdatasrc::gatherBaseTypeNames_Member(crow::json::rvalue& member, std::string prefix,
                                            std::vector<pair<std::string, std::string> >& names)
{
    if (member.t() == crow::json::type::Object) {
        gatherBaseTypeNames(member, prefix + ".", names);
    } else if (member.t() == crow::json::type::List) {
        gatherBaseTypeNames(member, prefix, names);
    } else {
        symbolMetadata& info = impl->findInfo(prefix);
        //Can't write a size 0
        if (info.size == 0) {
            cout << "Can't write size 0: " << prefix << ", Skipping" << endl;
        }
        //Can't write a single structure that is greater than 8 bytes with no members
        else if ((info.size > 8) && (info.dataType == 65)) {
            cout << "Can't write single structure greater than 8 with no members: " << prefix << ", Skipping" << endl;
        }
        //We can write this! Add it to the list
        else {
            names.emplace_back(make_pair(prefix, std::string(member)));
        }
    }
}

crow::json::wvalue adsdatasrc::getSymbolValue(std::string symbolName)
{
    return impl->findValue(symbolName);
}
