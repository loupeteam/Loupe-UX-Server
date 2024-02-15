
#include <unordered_map>

#include "DataSource.h"
#include "DataSource_impl.h"
#include "util.h"

using namespace std;

adsdatasrc_impl::~adsdatasrc_impl()
{
    delete this->parsedSymbols;
    for (auto& dt : this->dataTypes) {
        delete dt.second;
    }
}

bool adsdatasrc_impl::getMemberInfo(string targetSymbol, CAdsSymbolInfo main)
{
    if (!main.m_pEntry) {
        main.m_pEntry = parsedSymbols->GetTypeByName(main.type);
    }

    if (main.m_pEntry) {
        return getMemberInfo(targetSymbol, main.m_pEntry, main.fullname, main.iGrp, main.iOffs);
    }
    return false;
}

bool adsdatasrc_impl::getMemberInfo(string            targetSymbol,
                                    PAdsDatatypeEntry Entry,
                                    string            prefix,
                                    unsigned long     group,
                                    uint32_t          offset)
{
    bool cacheComplete = true;
    CAdsSymbolInfo info;
    for (UINT i = 0; i < parsedSymbols->SubSymbolCount(Entry); i++) {
        parsedSymbols->SubSymbolInfo(Entry, i, info);
        if (prefix != "") {
            if (Entry->arrayDim == 0) {
                info.fullname = prefix + "." + info.name;
            } else {
                info.fullname = prefix + "[" + info.name + "]";
            }
        } else {
            info.fullname = info.name;
        }

        //If the shorter of target symbol and current symbol is not in the other one, then skip
        // This is so we don't cache large strcutres that are not needed
        if (targetSymbol.size() < info.fullname.size()) {
            if (targetSymbol.compare(0, targetSymbol.size(), info.fullname, 0, targetSymbol.size()) != 0) {
                cacheComplete = false;
                continue;
            }
        } else {
            if (info.fullname.compare(0, info.fullname.size(), targetSymbol, 0, info.fullname.size()) != 0) {
                cacheComplete = false;
                continue;
            }
        }

        symbolMetadata& symbol = this->symbolInfo[info.fullname];

        populateSymbolInfo(symbol, info.fullname, group, offset, info);

        AdsDatatypeEntry SubEntry;
        parsedSymbols->SubSymbolEntry(Entry, i, SubEntry);
        if (SubEntry.subItems == 0) {
            info.m_pEntry = parsedSymbols->GetTypeByName(info.type);
            if (info.m_pEntry) {
                symbol.cacheComplete = getMemberInfo(targetSymbol,
                                                     info.m_pEntry,
                                                     info.fullname,
                                                     group,
                                                     offset + SubEntry.offs);
            }
        } else {
            symbol.cacheComplete = getMemberInfo(targetSymbol,
                                                 &SubEntry,
                                                 info.fullname,
                                                 group,
                                                 offset + SubEntry.offs);
        }
        cacheComplete = cacheComplete && symbol.cacheComplete;
    }
    return cacheComplete;
}

bool adsdatasrc_impl::supportType(ULONG flags)
{
    return !(flags & ADSDATATYPEFLAG_REFERENCETO) && !(flags & ADSDATATYPEFLAG_METHODDEREF) &&
           !(flags & ADSDATATYPEFLAG_PROPITEM) && !(flags & ADSDATATYPEFLAG_DATATYPE) &&
           !(flags & ADSDATATYPEFLAG_TCCOMIFACEPTR);
}

long adsdatasrc_impl::readInfo()
{
    AdsSymbolUploadInfo2 info;
	uint32_t bytesRead;

    long nResult = route->ReadReqEx2(ADSIGRP_SYM_UPLOADINFO2, 0, sizeof(info), &info, &bytesRead);

    if (nResult == ADSERR_NOERR) {
        // size of symbol information
        PBYTE pSym = new BYTE[info.nSymSize];
        PBYTE pDT = new BYTE[info.nDatatypeSize];

        if (pSym && pDT) {
            // upload symbols (instances)
            long resultSym = route->ReadReqEx2(ADSIGRP_SYM_UPLOAD, 0, info.nSymSize, pSym, &bytesRead);
            // get size of datatype description
            // upload datatye-descriptions
            long resultDt = route->ReadReqEx2(ADSIGRP_SYM_DT_UPLOAD, 0, info.nDatatypeSize, pDT, &bytesRead);

            this->parsedSymbols = new CAdsParseSymbols(pSym, info.nSymSize, pDT,
                                                       info.nDatatypeSize);
            this->ready = true;
        }
        delete[] pSym;
        delete[] pDT;
    }
    return nResult;
}

bool adsdatasrc_impl::cacheSymbolInfo(string symbolName)
{
    CAdsSymbolInfo Entry;
    this->parsedSymbols->Symbol(symbolName, Entry);
    if (Entry.valid == false) {
        return false;
    }
    symbolMetadata& info = this->symbolInfo[Entry.name];
    populateSymbolInfo(info, Entry.fullname, Entry);
    info.cacheComplete = this->getMemberInfo(symbolName, Entry);
    return true;
}

void adsdatasrc_impl::parseBuffer(crow::json::wvalue& variable,
                                  symbolMetadata&     datatype,
                                  void*               pBuffer,
                                  unsigned long       size)
{
    unsigned char* buffer = (unsigned char*)pBuffer;

    //If this is a basic data type, then we can parse it with the given parser
    if (datatype.memberCount() == 0) {
        // Read failed means that we attempted to read it and go an error
        //  This usually means that the variable is not available
        if (datatype.readFail == 1) {
            //Not Parsed
            variable.clear();
        }
        //If this is a property item, then we need to to get a handle to it
        // before we can read it
        else if ((datatype.flags_struct.PROPITEM == true) && (datatype.handle == 0)) {
            //Not Parsed
            variable.clear();
            //If we have not read this property yet, then we need to read it
            this->propertyReads.push_back(datatype.name);
        } else {
            if (!dataType_member_base::parse(datatype.dataType, variable, buffer, size)) {
                //Not Parsed
                variable.clear();
            } else {
                //Parsed
            }
        }
    } else {
        //If this has members, we need to go through them and parse them
        int i = 0;
        for ( auto &member : datatype.members()) {
            crow::json::wvalue& var = datatype.isArray ? variable[i] : variable[member.first];
            parseBuffer(var,
                        *member.second,
                        buffer + member.second->offset,
                        member.second->size);
            i++;
        }
    }
}

bool adsdatasrc_impl::encodeBuffer(string&       variable,
                                   void*         pBuffer,
                                   string&       value,
                                   unsigned long size)
{
    BYTE* buffer = (BYTE*)pBuffer;

    symbolMetadata datatype = this->findInfo(variable);

    if (datatype.valid == true) {
        //If this is a basic data type, then we can parse it with the given parser
        if (datatype.memberCount() == 0) {
            if (!dataType_member_base::encode(datatype.dataType, buffer, value, datatype.size)) {
                return false;
            }
        }
        //If this has members, they should have been broken up into individual variables.
        //  For shame..
        else {
            return false;
        }
    }
    //The variable was not found
    else {
        return false;
    }
    //All is well
    return true;
}

symbolMetadata& adsdatasrc_impl::findInfo(string& symbolName)
{
    symbolMetadata& info = this->symbolInfo[symbolName];
    if ((info.cacheComplete == false) && !info.notFound) {
        info.notFound = !cacheSymbolInfo(symbolName);
    }
    return info;
}

crow::json::wvalue& adsdatasrc_impl::findValue(string& symbolName)
{
    crow::json::wvalue& ret = find(symbolName, this->symbolData);
    return ret;
}

crow::json::wvalue& adsdatasrc_impl::find(string symbolName, crow::json::wvalue& datasource)
{
    toLower(symbolName);
    deque<string> path = splitVarName(symbolName, ".[");
    crow::json::wvalue* ret = &datasource[path[0]];
    for ( size_t i = 1; i < path.size(); i++) {
        crow::json::wvalue* data;
        data = ret;
        ret = &(*data)[path[i]];
    }
    return *ret;
}

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                         string&         symbolName,
                                         unsigned long   parentGroup,
                                         unsigned long   parentOffset,
                                         CAdsSymbolInfo& info)
{
    symbol.name = symbolName;
    symbol.group = parentGroup;
    symbol.gOffset = parentOffset + info.offs;
    symbol.offset = info.offs;
    symbol.size = info.size;
    symbol.type = info.type;
    symbol.dataType = info.dataType;
    symbol.comment = info.comment;
    symbol.flags = info.flags;
    symbol.flags_struct = datatype_flags_struct(info.flags, false);
    symbol.isArray = info.arrayDim > 0;
    symbol.valid = !info.flags_struct.TCCOMIFACEPTR && !info.flags_struct.REFERENCETO;
}

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                         string&         symbolName,
                                         CAdsSymbolInfo& info)
{
    symbol.name = symbolName;
    symbol.group = info.iGrp;
    symbol.gOffset = info.iOffs;
    symbol.offset = info.offs;
    symbol.size = info.size;
    symbol.type = info.type;
    symbol.dataType = info.dataType;
    symbol.comment = info.comment;
    symbol.flags = info.flags;
    symbol.flags_struct = datatype_flags_struct(info.flags, true);
    symbol.isArray = info.arrayDim > 0;
    symbol.valid = !info.flags_struct.TCCOMIFACEPTR && !info.flags_struct.REFERENCETO;
}

PAdsSymbolEntry adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                                    string&         symbolName,
                                                    PAdsSymbolEntry pAdsSymbolEntry)
{
    symbol.name = symbolName;
    symbol.group = pAdsSymbolEntry->iGroup;
    symbol.gOffset = pAdsSymbolEntry->iOffs;
    symbol.offset = 0;
    symbol.size = pAdsSymbolEntry->size;
    symbol.type = PADSSYMBOLTYPE(pAdsSymbolEntry);
    symbol.dataType = pAdsSymbolEntry->dataType;
    symbol.comment = PADSSYMBOLCOMMENT(pAdsSymbolEntry);
    symbol.valid = true;
    symbol.flags = pAdsSymbolEntry->flags;
    symbol.flags_struct = datatype_flags_struct(pAdsSymbolEntry->flags, true);
    // symbol.isArray = pAdsSymbolEntry-> > 0;
    symbol.valid = !symbol.flags_struct.TCCOMIFACEPTR && !symbol.flags_struct.REFERENCETO;
    return pAdsSymbolEntry;
}
