
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

void adsdatasrc_impl::getMemberInfo(std::string targetSymbol, CAdsSymbolInfo main)
{
    if (!main.m_pEntry) {
        main.m_pEntry = parsedSymbols->GetTypeByName(main.type);
    }

    if (main.m_pEntry) {
        getMemberInfo(targetSymbol, main.m_pEntry, main.fullname, main.iGrp, main.iOffs);
    }
}

void adsdatasrc_impl::getMemberInfo(std::string       targetSymbol,
                                    PAdsDatatypeEntry Entry,
                                    string            prefix,
                                    unsigned long     group,
                                    uint32_t          offset)
{
    CAdsSymbolInfo info;
    for (size_t i = 0; i < parsedSymbols->SubSymbolCount(Entry); i++) {
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
                continue;
            }
        } else {
            if (info.fullname.compare(0, info.fullname.size(), targetSymbol, 0, info.fullname.size()) != 0) {
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
                getMemberInfo(targetSymbol, info.m_pEntry, info.fullname, group, offset + SubEntry.offs);
            }
        } else {
            getMemberInfo(targetSymbol, &SubEntry, info.fullname, group, offset + SubEntry.offs);
        }
    }
}

void adsdatasrc_impl::prepareDatatypeParser(dataType_member_base* parser)
{
    PAdsDatatypeEntry pAdsDatatypeEntry = this->parsedSymbols->GetTypeByName(parser->type);
    parser->isArray = pAdsDatatypeEntry->arrayDim > 0;
    for (int j = 0; j < this->parsedSymbols->SubSymbolCount(pAdsDatatypeEntry); j++) {
        CAdsSymbolInfo info;
        this->parsedSymbols->SubSymbolInfo(pAdsDatatypeEntry, j, info);
        //TODO: Some basic types that aren't support should be..
        //  Like ENUM, HRESULT, etc
//        if( supportType( info.flags ) && info.size > 0 ){
        auto member = datatype_member::parserForType(info.name, info.type, info.offs, info.size);
        parser->members.push_back(member);
//        }
    }
    parser->valid = true;
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
    long nResult = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_UPLOADINFO2, 0,
                                  sizeof(info), &info);

    if (nResult == ADSERR_NOERR) {
        // size of symbol information
        PBYTE pSym = new BYTE[info.nSymSize];
        PBYTE pDT = new BYTE[info.nDatatypeSize];

        if (pSym && pDT) {
            // upload symbols (instances)
            long resultSym = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_UPLOAD, 0,
                                            info.nSymSize, pSym);
            // get size of datatype description
            // upload datatye-descriptions
            long resultDt = AdsSyncReadReq(this->pAddr, ADSIGRP_SYM_DT_UPLOAD, 0,
                                           info.nDatatypeSize, pDT);

            this->parsedSymbols = new CAdsParseSymbols(pSym, info.nSymSize, pDT,
                                                       info.nDatatypeSize);
            this->ready = true;
        }
        delete[] pSym;
        delete[] pDT;
    }
    return nResult;
}

void adsdatasrc_impl::cacheDataTypes()
{
    UINT count = this->parsedSymbols->DatatypeCount();
    for (int i = 0; i < count; i++) {
        PAdsDatatypeEntry pAdsDatatypeEntry = this->parsedSymbols->GetTypeByIndex(i);
        string typeName = PADSDATATYPENAME(pAdsDatatypeEntry);
        dataType_member_base* dt = this->getType(typeName);
        if (dt->valid) {
            continue;
        }
        dt->name = typeName;
    }
}

bool adsdatasrc_impl::cacheSymbolInfo(std::string symbolName)
{
    CAdsSymbolInfo Entry;
    this->parsedSymbols->Symbol(symbolName, Entry);
    if (Entry.valid == false) {
        return false;
    }
    symbolMetadata& info = this->symbolInfo[Entry.name];
    populateSymbolInfo(info, Entry.fullname, Entry);
    this->getMemberInfo(symbolName, Entry);
    return true;
}

void adsdatasrc_impl::parseBuffer(crow::json::wvalue& variable, string& datatype, void* pBuffer, unsigned long size)
{
    byte* buffer = (byte*)pBuffer;

    dataType_member_base* dt = this->getType(datatype);

    if (!dt->valid) {
        this->prepareDatatypeParser(dt);
    }

    //If this is a basic data type, then we can parse it with the given parser
    //Otherwise go through the members and parse them
    if (dt->baseType) {
        // datatype_member_base::parse(variable, dt->datatype, buffer, size);

        variable.empty_object();
        dt->parse(variable, buffer, size);
        return;
    } else {
        if (dt->members.size() == 0) {
            variable.empty_object();
        } else {
            for ( auto member : dt->members ) {
                parseBuffer(variable[member->name], member->type, buffer + member->offset, member->size);
            }
        }
    }
}

void adsdatasrc_impl::parseBuffer(crow::json::wvalue& variable,
                                  symbolMetadata&     datatype,
                                  void*               pBuffer,
                                  unsigned long       size)
{
    byte* buffer = (byte*)pBuffer;

    //If this is a basic data type, then we can parse it with the given parser
    if (datatype.memberCount() == 0) {
        if (datatype.flags_struct.PROPITEM) {
            //Not Parsed
            variable = "Property Item: " + datatype.type;
        } else if (datatype.flags_struct.DATATYPE) {
            //Not Parsed
            variable = "Datatype: " + datatype.type;
        } else {
            if (!dataType_member_base::parse(datatype.dataType, variable, buffer, size)) {
                //Not Parsed
                variable = "Not Parsed: " + datatype.type;
            }
        }
    } else {
        //If this has members, we need to go through them and parse them
        for ( auto member : datatype.members()) {
            parseBuffer(variable[member.first],
                        member.second,
                        buffer + member.second.offset,
                        member.second.size);
        }
    }
}

bool adsdatasrc_impl::encodeBuffer(std::string&  variable,
                                   void*         pBuffer,
                                   std::string&  value,
                                   unsigned long size)
{
    BYTE* buffer = (BYTE*)pBuffer;

    symbolMetadata datatype = this->findInfo(variable);

    if (datatype.valid == true) {
        //If this is a basic data type, then we can parse it with the given parser
        if (datatype.memberCount() == 0) {
            if (datatype.flags_struct.PROPITEM) {
                return false;
            } else if (datatype.flags_struct.DATATYPE) {
                return false;
            } else {
                if (!dataType_member_base::encode(datatype.dataType, buffer, value, datatype.size)) {
                    return false;
                }
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

dataType_member_base* adsdatasrc_impl::getType(std::string& typeName)
{
    dataType_member_base* dt = this->dataTypes[typeName];
    if (dt == NULL) {
        dt = dataType_member_base::parserForType("", typeName, 0, 0);
        dt->valid = false;
        this->dataTypes[typeName] = dt;
    }
    return dt;
}

symbolMetadata& adsdatasrc_impl::findInfo(std::string& symbolName)
{
    symbolMetadata& info = this->symbolInfo[symbolName];
    if ((info.valid == false) && !info.notFound) {
        info.notFound = !cacheSymbolInfo(symbolName);
    }
    return info;
}

crow::json::wvalue& adsdatasrc_impl::findValue(std::string& symbolName)
{
    crow::json::wvalue& ret = find(symbolName, this->symbolData);
    return ret;
}

crow::json::wvalue& adsdatasrc_impl::find(std::string symbolName, crow::json::wvalue& datasource)
{
    toLower(symbolName);
    std::deque<std::string> path = split(symbolName, ".[");
    crow::json::wvalue* ret = &datasource[path[0]];
    for ( int i = 1; i < path.size(); i++) {
        crow::json::wvalue* data;
        data = ret;
        ret = &(*data)[path[i]];
    }
    return *ret;
}

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                         std::string&    symbolName,
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

    symbol.valid = !info.flags_struct.TCCOMIFACEPTR && !info.flags_struct.REFERENCETO && !info.flags_struct.DATATYPE;
}

void adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                         std::string&    symbolName,
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
    symbol.valid = !info.flags_struct.TCCOMIFACEPTR && !info.flags_struct.REFERENCETO && !info.flags_struct.DATATYPE;
}

PAdsSymbolEntry adsdatasrc_impl::populateSymbolInfo(symbolMetadata& symbol,
                                                    std::string&    symbolName,
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
    symbol.valid = !symbol.flags_struct.TCCOMIFACEPTR && !symbol.flags_struct.REFERENCETO &&
                   !symbol.flags_struct.DATATYPE;
    return pAdsSymbolEntry;
}
