#include "DataSource.h"
#include "DataSource_impl.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <iostream>

#define impl ((adsdatasrc_impl*)_impl)

using namespace std;

//The static impl is just used for to make debugging easier
adsdatasrc_impl* static_impl;

adsdatasrc::adsdatasrc()
{
    _impl = new adsdatasrc_impl();
    static_impl = impl;
}

adsdatasrc::~adsdatasrc()
{
    delete impl;
}


void adsdatasrc::setPlcCommunicationParameters( std::string IpV4,std::string netId, uint16_t port)
{

    const int REQUIRED_NUM_NET_ID_PARTS = 6;
    const int NETID_PART_MIN = 0;
    const int NETID_PART_MAX = 255;
    unsigned char tempNetIdPartArr[REQUIRED_NUM_NET_ID_PARTS];
    AmsNetId Addr;
    std::deque<std::string> netIdParts = split(netId, '.');

    // Check Deque size
    if (netIdParts.size() != REQUIRED_NUM_NET_ID_PARTS) {
        cerr << "Could not process netID.";
        return;
    }

    // Convert each part into integer. Check if within limits.
    for (int i = 0; i < REQUIRED_NUM_NET_ID_PARTS; i++) {
        
        try {
            std::string temp = netIdParts.front();
            int n = std::stoi(temp, nullptr);

            if ((n >= NETID_PART_MIN) && (n <= NETID_PART_MAX)) {
                tempNetIdPartArr[i] = n;
            }
            else {
                cerr << "Out-of-range part of netID.";
                return;
            }
            
            netIdParts.pop_front();
        }
        catch(...) {
            cerr << "Could not process part of netID.";
            return;
        } 
    }
    
    // If no errors, assign netID parts
    for (int i = 0; i < REQUIRED_NUM_NET_ID_PARTS; i++) {
        Addr.b[i] = tempNetIdPartArr[i];
    }

    impl->route = std::make_shared<AdsDevice>(AdsDevice{IpV4, Addr, port});
}

void adsdatasrc::setRouter(void *router){
    
    impl->route = *static_cast<std::shared_ptr<AdsDevice>*>(router);
}

//Set the local ams net id from a string
void adsdatasrc::setLocalAms(std::string netId)
{
    const int REQUIRED_NUM_NET_ID_PARTS = 6;
    const int NETID_PART_MIN = 0;
    const int NETID_PART_MAX = 255;
    unsigned char tempNetIdPartArr[REQUIRED_NUM_NET_ID_PARTS];

    std::deque<std::string> netIdParts = split(netId, '.');
    
    AmsNetId localAmsNetId;

    // Check Deque size
    if (netIdParts.size() != REQUIRED_NUM_NET_ID_PARTS) {
        cerr << "Could not process netID.";
        return;
    }

    // Convert each part into integer. Check if within limits.
    for (int i = 0; i < REQUIRED_NUM_NET_ID_PARTS; i++) {
        
        try {
            std::string temp = netIdParts.front();
            int n = std::stoi(temp, nullptr);

            if ((n >= NETID_PART_MIN) && (n <= NETID_PART_MAX)) {
                tempNetIdPartArr[i] = n;
            }
            else {
                cerr << "Out-of-range part of netID.";
                return;
            }
            
            netIdParts.pop_front();
        }
        catch(...) {
            cerr << "Could not process part of netID.";
            return;
        } 
    }
    
    // If no errors, assign netID parts
    for (int i = 0; i < REQUIRED_NUM_NET_ID_PARTS; i++) {
        localAmsNetId.b[i] = tempNetIdPartArr[i];
    }
    bhf::ads::SetLocalAddress(localAmsNetId);
}

//Read the symbol and datatype data from the PLC
void adsdatasrc::readPlcData()
{    
    while (impl->readInfo() != 0) {
        #ifdef WIN32
        Sleep(1000);
        #else
        sleep(1000);
        #endif
        cerr << "No PLC Connections, will try again " << '\n';
    }
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
    uint32_t bytesRead;
    if (info.flags_struct.PROPITEM) {
        // Read value of a PLC variable (by handle)
        nResult = impl->route->ReadReqEx2(ADSIGRP_SYM_VALBYHND, info.handle, size, buffer, &bytesRead);
    } else {
        // Read a variable from ADS
        nResult = impl->route->ReadReqEx2(info.group, info.offset, size, buffer, &bytesRead);
    }

    //Parse the buffer into the variable
    if (nResult == ADSERR_NOERR) {
        info.readFail = 0;
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
            info.readFail = 1;
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
    size_t reqNum = symbolNames.size();
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
    uint32_t bytesRead;
    // Read a variable from ADS
    long nResult = impl->route->ReadWriteReqEx2(
        ADSIGRP_SUMUP_READ, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        4 * reqNum + size, // we request additional "error"-flag(long) for each ADS-sub commands
        buffer,          // pointer to buffer for ADS-data
        12 * reqNum, // send 12 bytes (3 * long : IG, IO, Len) of each ADS-sub command
        parReq,   // pointer to buffer for ADS-commands
        &bytesRead);
    if (nResult == ADSERR_NOERR) {
        PBYTE pObjAdsRes = (BYTE*)buffer + (reqNum * 4); // point to ADS-data
        PBYTE pObjAdsErrRes = (BYTE*)buffer;          // point to ADS-err
        PBYTE pdata = pObjAdsRes;
        for (auto symbolName : symbolNames) {
            uint32_t result = *(uint32_t*)pObjAdsErrRes;
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
                    info.readFail = 1;
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

void adsdatasrc::readSymbolValueDirect(std::string symbolName){
    AdsVariable<float> read_var{*impl->route, symbolName};
    float value = read_var;
}

//Write the value of a json packet to the PLC
void adsdatasrc::writeSymbolValue(crow::json::rvalue packet)
{
    if (!ready()) {
        return;
    }

    std::vector<pair<std::string, std::string> > symbolNames;
    string prefix = "";
    gatherBaseTypeNames(packet, prefix, symbolNames);

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

    PBYTE mAdsSumBufferRes = new BYTE[reqNum * 4];
    uint32_t bytesRead;
    // Read a variable from ADS
    long nResult = impl->route->ReadWriteReqEx2(
        ADSIGRP_SUMUP_WRITE, // Sum-Command, response will contain ADS-error code for each ADS-Sub-command
        reqNum, // Number of ADS-Sub-commands
        reqNum * 4, // we request additional "error"-flag(long) for each ADS-sub commands
        mAdsSumBufferRes,          // pointer to buffer for ADS-data
        reqNum * sizeof(dataPar) + size,      // send x bytes (3 * long : IG, IO, Len) + data command
        buffer,   // pointer to buffer for ADS-commands
        &bytesRead);
    if (nResult == ADSERR_NOERR) {
        uint32_t* pResult = (uint32_t*)mAdsSumBufferRes;
        //Output the individual statuses
        for (auto symbolName : symbolNames) {
            uint32_t result = *pResult;
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
        uint32_t bytesRead;
        // Fetch handle for an <szVar> PLC variable
        long nResult = impl->route->ReadWriteReqEx2(ADSIGRP_SYM_HNDBYNAME,
                                            0x0,
                                            sizeof(info.handle),
                                            &info.handle,
                                            info.name.size(),
                                            (void*)info.name.c_str(),
                                            &bytesRead);
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
        string name = prefix + ".";
        gatherBaseTypeNames(member, name, names);
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
