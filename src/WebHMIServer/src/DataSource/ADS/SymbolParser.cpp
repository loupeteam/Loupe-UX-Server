// AdsParseSymbols.cpp: implementation of the CAdsParseSymbols class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "DataSource_impl.h"
#include "SymbolParser.h"
#include "util.h"

#define ASSERT(x)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// CAdsParseSymbols

CAdsParseSymbols::CAdsParseSymbols(PVOID pSymbols, UINT nSymSize, PVOID pDatatypes, UINT nDTSize)
{
    memset(m_bufGetTypeByNameBuffer, 0, sizeof(m_bufGetTypeByNameBuffer));
    m_pSymbols = new BYTE[nSymSize + sizeof(ULONG)];
    if (m_pSymbols) {
        m_nSymSize = nSymSize;
        if (nSymSize) {
            memcpy(m_pSymbols, pSymbols, nSymSize);
        }
        *(PULONG)&m_pSymbols[nSymSize] = 0;
        m_nSymbols = 0;
        UINT offs = 0;
        while (*(PULONG)&m_pSymbols[offs]) {
            m_nSymbols++;
            offs += *(PULONG)&m_pSymbols[offs];
        }
        ASSERT(offs == nSymSize);
        m_ppSymbolArray = new PAdsSymbolEntry[m_nSymbols];
        m_nSymbols = offs = 0;
        while (*(PULONG)&m_pSymbols[offs]) {
            m_ppSymbolArray[m_nSymbols++] = (PAdsSymbolEntry) & m_pSymbols[offs];
            offs += *(PULONG)&m_pSymbols[offs];
        }
        ASSERT(offs == nSymSize);
    }

    m_pDatatypes = new BYTE[nDTSize + sizeof(ULONG)];
    if (m_pDatatypes) {
        m_nDTSize = nDTSize;
        if (nDTSize) {
            memcpy(m_pDatatypes, pDatatypes, nDTSize);
        }
        *(PULONG)&m_pDatatypes[nDTSize] = 0;
        m_nDatatypes = 0;
        UINT offs = 0;
        while (*(PULONG)&m_pDatatypes[offs]) {
            m_nDatatypes++;
            offs += *(PULONG)&m_pDatatypes[offs];
        }
        ASSERT(offs == nDTSize);
        m_ppDatatypeArray = new PAdsDatatypeEntry[m_nDatatypes];
        m_nDatatypes = offs = 0;
        while (*(PULONG)&m_pDatatypes[offs]) {
            m_ppDatatypeArray[m_nDatatypes++] = (PAdsDatatypeEntry) & m_pDatatypes[offs];
            offs += *(PULONG)&m_pDatatypes[offs];
        }
        ASSERT(offs == nDTSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
CAdsParseSymbols::~CAdsParseSymbols()
{
    if (m_pSymbols) {
        delete m_pSymbols;
    }
    if (m_pDatatypes) {
        delete m_pDatatypes;
    }
    if (m_ppSymbolArray) {
        delete m_ppSymbolArray;
    }
    if (m_ppDatatypeArray) {
        delete m_ppDatatypeArray;
    }
}

PAdsDatatypeEntry CAdsParseSymbols::GetTypeByName(std::string sType)
{
    PAdsDatatypeEntry pKey = (PAdsDatatypeEntry)m_bufGetTypeByNameBuffer;

    //Go through the data types and find the one that matches the name
    for ( unsigned int i = 0; i < m_nDatatypes; i++ ) {
        if (sType == PADSDATATYPENAME(m_ppDatatypeArray[i])) {
            return m_ppDatatypeArray[i];
        }
    }
    return NULL;
}

PAdsDatatypeEntry CAdsParseSymbols::GetTypeByIndex(UINT sym)
{
    if (sym < m_nDatatypes) {
        return m_ppDatatypeArray[sym];
    } else {
        return NULL;
    }
}

UINT CAdsParseSymbols::SubSymbolCount(UINT sym)
{
    if (sym < m_nSymbols) {
        return SubSymbolCount(SymbolType(sym));
    } else {
        return 0;
    }
}

UINT CAdsParseSymbols::SubSymbolCount(PCHAR sType)
{
    PAdsDatatypeEntry pEntry = GetTypeByName(sType);
    if (pEntry) {
        return SubSymbolCount(pEntry);
    } else {
        return 0;
    }
}

UINT CAdsParseSymbols::SubSymbolCount(CAdsSymbolInfo& Symbol)
{
    if (!Symbol.m_pEntry) {
        Symbol.m_pEntry = GetTypeByName(Symbol.type);
    }

    return SubSymbolCount(Symbol.m_pEntry);
}

UINT CAdsParseSymbols::SubSymbolCount(PAdsDatatypeEntry pEntry)
{
    UINT cnt = 0;
    if (pEntry) {
        if (pEntry->subItems) {
            cnt += pEntry->subItems;
        } else if (pEntry->arrayDim) {
            cnt = 1;
            PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
            for ( USHORT i = 0; i < pEntry->arrayDim; i++ ) {
                cnt *= pAI[i].elements;
            }
        }
    }

    return cnt;
}

BOOL CAdsParseSymbols::Symbol(UINT sym, CAdsSymbolInfo& info)
{
    PAdsSymbolEntry pEntry = Symbol(sym);
    if (pEntry == NULL) {
        return FALSE;
    }
    info.iGrp = pEntry->iGroup;
    info.iOffs = pEntry->iOffs;
    info.size = pEntry->size;
    info.dataType = pEntry->dataType;
    info.flags = pEntry->flags;
    info.name = PADSSYMBOLNAME(pEntry);
    info.fullname = PADSSYMBOLNAME(pEntry);
    info.type = PADSSYMBOLTYPE(pEntry);
    info.comment = PADSSYMBOLCOMMENT(pEntry);
    info.flags_struct = datatype_flags_struct(pEntry->flags, true);
    info.m_pEntry = GetTypeByName(info.type);
    if (info.m_pEntry) {
        info.arrayDim = info.m_pEntry->arrayDim;
    }
    return TRUE;
}

BOOL CAdsParseSymbols::Symbol(std::string name, CAdsSymbolInfo& info)
{
    int i = 0;
    PAdsSymbolEntry pEntry = NULL;
    toLower(name);
    while (pEntry = Symbol(i++)) {
        std::string symbolName = PADSSYMBOLNAME(pEntry);
        toLower(symbolName);
        if (name._Starts_with(symbolName)) {
            break;
        }
    }
    if (pEntry == NULL) {
        info.valid = false;
        return FALSE;
    }
    info.valid = true;
    info.offs = 0;
    info.iGrp = pEntry->iGroup;
    info.iOffs = pEntry->iOffs;
    info.size = pEntry->size;
    info.dataType = pEntry->dataType;
    info.flags = pEntry->flags;
    info.name = PADSSYMBOLNAME(pEntry);
    info.fullname = PADSSYMBOLNAME(pEntry);
    info.type = PADSSYMBOLTYPE(pEntry);
    info.comment = PADSSYMBOLCOMMENT(pEntry);
    info.flags_struct = datatype_flags_struct(pEntry->flags, true);
    info.m_pEntry = GetTypeByName(info.type);
    if (info.m_pEntry) {
        info.arrayDim = info.m_pEntry->arrayDim;
    }

    return TRUE;
}

BOOL CAdsParseSymbols::SubSymbolInfo(CAdsSymbolInfo& main, UINT sub, CAdsSymbolInfo& info)
{
    if (!main.m_pEntry) {
        main.m_pEntry = GetTypeByName(main.type);
    }
    PAdsDatatypeEntry pEntry = main.m_pEntry;
    if (pEntry) {
        if (pEntry->subItems) {
            PAdsDatatypeEntry pSEntry = AdsDatatypeStructItem(pEntry, sub);
            if (pSEntry) {
                info.iGrp = main.iGrp;
                info.iOffs = main.iOffs + pSEntry->offs;
                info.size = pSEntry->size;
                info.dataType = pSEntry->dataType;
                info.flags = pSEntry->flags;
                info.name = PADSDATATYPENAME(pSEntry);
                info.fullname = main.fullname + "." + info.name;
                info.type = PADSDATATYPETYPE(pSEntry);
                info.comment = PADSDATATYPECOMMENT(pSEntry);
                info.flags_struct = datatype_flags_struct(pSEntry->flags, true);
                info.m_pEntry = GetTypeByName(info.type);
                if (info.m_pEntry) {
                    info.arrayDim = info.m_pEntry->arrayDim;
                }
                return TRUE;
            }
        } else if (pEntry->arrayDim) {
            UINT x[10] = {0}, baseSize = pEntry->size;
            x[pEntry->arrayDim] = 1;
            PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
            for ( int i = pEntry->arrayDim - 1; i >= 0; i-- ) {
                x[i] = x[i + 1] * pAI[i].elements;
                if (pAI[i].elements) {
                    baseSize /= pAI[i].elements;
                }
            }
            if ((sub == 0) && (x[0] > 1000)) {
                std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
            }
            if (sub < x[0]) {
                info.iGrp = main.iGrp;
                info.iOffs = main.iOffs;
                info.size = baseSize;
                info.dataType = pEntry->dataType;
                info.flags = pEntry->flags;
                info.type = PADSDATATYPETYPE(pEntry);
                info.comment = PADSDATATYPECOMMENT(pEntry);
                std::string arr = "[";
                for ( int i = 0; i < pEntry->arrayDim; i++ ) {
                    arr += pAI[i].lBound + sub / x[i + 1];
                    arr += (i == pEntry->arrayDim - 1) ? ']' : ',';
                }
                info.name = main.name + arr;
                info.fullname = main.fullname + arr;
                info.flags_struct = datatype_flags_struct(pEntry->flags, false);
                info.m_pEntry = GetTypeByName(info.type);
                if (info.m_pEntry) {
                    info.arrayDim = info.m_pEntry->arrayDim;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CAdsParseSymbols::SubSymbolInfo(PAdsDatatypeEntry Entry, UINT sub, CAdsSymbolInfo& info)
{
    PAdsDatatypeEntry pEntry = Entry;
    if (pEntry) {
        if (pEntry->subItems) {
            PAdsDatatypeEntry pSEntry = AdsDatatypeStructItem(pEntry, sub);
            if (pSEntry) {
                info.iGrp = 0;
                info.iOffs = 0;
                info.offs = pSEntry->offs;
                info.size = pSEntry->size;
                info.dataType = pSEntry->dataType;
                info.flags = pSEntry->flags;
                info.name = PADSDATATYPENAME(pSEntry);
                info.fullname = info.name;
                info.type = PADSDATATYPETYPE(pSEntry);
                info.comment = PADSDATATYPECOMMENT(pSEntry);
                info.flags_struct = datatype_flags_struct(pSEntry->flags, false);
                info.m_pEntry = GetTypeByName(info.type);
                if (info.m_pEntry) {
                    info.arrayDim = info.m_pEntry->arrayDim;
                }
                return TRUE;
            }
        } else if (pEntry->arrayDim) {
            UINT x[10] = {0}, baseSize = pEntry->size;
            x[pEntry->arrayDim] = 1;
            PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
            for ( int i = pEntry->arrayDim - 1; i >= 0; i-- ) {
                x[i] = x[i + 1] * pAI[i].elements;
                if (pAI[i].elements) {
                    baseSize /= pAI[i].elements;
                }
            }
            if ((sub == 0) && (x[0] > 1000)) {
                std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
            }
            if (sub < x[0]) {
                info.iGrp = 0;
                info.iOffs = 0;
                info.size = baseSize;
                info.dataType = pEntry->dataType;
                info.flags = pEntry->flags;
                info.type = PADSDATATYPETYPE(pEntry);
                info.comment = PADSDATATYPECOMMENT(pEntry);
                info.name = std::to_string(sub);
                info.fullname = std::string(PADSSYMBOLNAME(pEntry)) + info.name;
                info.flags_struct = datatype_flags_struct(pEntry->flags, false);
                info.m_pEntry = GetTypeByName(info.type);
                if (info.m_pEntry) {
                    info.arrayDim = info.m_pEntry->arrayDim;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CAdsParseSymbols::SubSymbolEntry(CAdsSymbolInfo& main, UINT sub, AdsDatatypeEntry& SEntry)
{
    if (!main.m_pEntry) {
        main.m_pEntry = GetTypeByName(main.type);
    }
    return SubSymbolEntry(main.m_pEntry, sub, SEntry);
}

BOOL CAdsParseSymbols::SubSymbolEntry(PAdsDatatypeEntry pEntry, UINT sub, AdsDatatypeEntry& SEntry)
{
    if (pEntry) {
        if (pEntry->subItems) {
            SEntry = *AdsDatatypeStructItem(pEntry, sub);
        } else if (pEntry->arrayDim) {
            UINT x[10] = {0}, baseSize = pEntry->size;
            x[pEntry->arrayDim] = 1;
            PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
            for ( int i = pEntry->arrayDim - 1; i >= 0; i-- ) {
                x[i] = x[i + 1] * pAI[i].elements;
                if (pAI[i].elements) {
                    baseSize /= pAI[i].elements;
                }
            }
            if ((sub == 0) && (x[0] > 1000)) {
                std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
            }
            if (sub < x[0]) {
                SEntry = *pEntry;
                std::string arr = "[";
                for ( int i = 0; i < pEntry->arrayDim; i++ ) {
//					arr	+= string::format("%d", pAI[i].lBound + sub / x[i+1]);
//					arr	+= (i==pEntry->arrayDim-1) ? ']' : ',';
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

void CAdsParseSymbols::DumpDatatypes()
{
    //Go through all the data types and COUT them
    for ( unsigned int i = 0; i < m_nDatatypes; i++ ) {
        std::cout << "Name: " << PADSDATATYPENAME(m_ppDatatypeArray[i]) << "\n";
        std::cout << "Type: " << PADSDATATYPETYPE(m_ppDatatypeArray[i]) << "\n";
        std::cout << "Comment: " << PADSDATATYPECOMMENT(m_ppDatatypeArray[i]) << "\n";
        std::cout << "Size: " << m_ppDatatypeArray[i]->size << "\n";
        std::cout << "SubItems: " << m_ppDatatypeArray[i]->subItems << "\n";
        std::cout << "ArrayDim: " << m_ppDatatypeArray[i]->arrayDim << "\n";
        std::cout << "Flags: " << m_ppDatatypeArray[i]->flags << "\n";
        std::cout << "DataType: " << m_ppDatatypeArray[i]->dataType << "\n";
        std::cout << "Offs: " << m_ppDatatypeArray[i]->offs << "\n";
        std::cout << "-----------------------------------\n";
    }
}

void CAdsParseSymbols::DumpSymbols()
{
    //Go through all the symbols and COUT them
    for ( unsigned int i = 0; i < m_nSymbols; i++ ) {
        std::cout << "Name: " << PADSSYMBOLNAME(m_ppSymbolArray[i]) << "\n";
        std::cout << "Type: " << PADSSYMBOLTYPE(m_ppSymbolArray[i]) << "\n";
        std::cout << "Comment: " << PADSSYMBOLCOMMENT(m_ppSymbolArray[i]) << "\n";
        std::cout << "Size: " << m_ppSymbolArray[i]->size << "\n";
//		std::cout << "SubItems: " << m_ppSymbolArray[i]->subItems << "\n";
//		std::cout << "ArrayDim: " << m_ppSymbolArray[i]->arrayDim << "\n";
        std::cout << "Flags: " << m_ppSymbolArray[i]->flags << "\n";
        std::cout << "DataType: " << m_ppSymbolArray[i]->dataType << "\n";
        std::cout << "Offs: " << m_ppSymbolArray[i]->iOffs << "\n";
        std::cout << "-----------------------------------\n";
    }
}

//              this->DATATYPE      = (flags & ADSSYMBOLFLAG_DATATYPE);
//		this->DATAITEM      = (flags & ADSSYMBOLFLAG_DATAITEM);
//		this->METHODDEREF   = (flags & ADSSYMBOLFLAG_METHODDEREF);
//		this->OVERSAMPLE    = (flags & ADSSYMBOLFLAG_OVERSAMPLE );
//		this->BITVALUES     = (flags & ADSSYMBOLFLAG_BITVALUES );
//		this->PROPITEM      = (flags & ADSSYMBOLFLAG_PROPITEM);
//		this->COPYMASK      = (flags & ADSSYMBOLFLAG_COPYMASK);
//		this->METHODINFOS   = (flags & ADSSYMBOLFLAG_METHODINFOS);
//		this->ATTRIBUTES    = (flags & ADSSYMBOLFLAG_ATTRIBUTES);
//		this->ENUMINFOS     = (flags & ADSSYMBOLFLAG_ENUMINFOS);
datatype_flags_struct::datatype_flags_struct(ULONG flags, bool symbol)
{
    if (symbol == true) {
        memset(this, 0, sizeof(datatype_flags_struct));
        this->REFERENCETO = (flags & ADSSYMBOLFLAG_REFERENCETO);
        this->TYPEGUID = (flags & ADSSYMBOLFLAG_TYPEGUID);
        this->PERSISTENT = (flags & ADSSYMBOLFLAG_PERSISTENT);
        this->TCCOMIFACEPTR = (flags & ADSSYMBOLFLAG_TCCOMIFACEPTR);
    } else {
        this->DATATYPE = (flags & ADSDATATYPEFLAG_DATATYPE);
        this->DATAITEM = (flags & ADSDATATYPEFLAG_DATAITEM);
        this->REFERENCETO = (flags & ADSDATATYPEFLAG_REFERENCETO);
        this->METHODDEREF = (flags & ADSDATATYPEFLAG_METHODDEREF);
        this->OVERSAMPLE = (flags & ADSDATATYPEFLAG_OVERSAMPLE);
        this->BITVALUES = (flags & ADSDATATYPEFLAG_BITVALUES);
        this->PROPITEM = (flags & ADSDATATYPEFLAG_PROPITEM);
        this->TYPEGUID = (flags & ADSDATATYPEFLAG_TYPEGUID);
        this->PERSISTENT = (flags & ADSDATATYPEFLAG_PERSISTENT);
        this->COPYMASK = (flags & ADSDATATYPEFLAG_COPYMASK);
        this->TCCOMIFACEPTR = (flags & ADSDATATYPEFLAG_TCCOMIFACEPTR);
        this->METHODINFOS = (flags & ADSDATATYPEFLAG_METHODINFOS);
        this->ATTRIBUTES = (flags & ADSDATATYPEFLAG_ATTRIBUTES);
        this->ENUMINFOS = (flags & ADSDATATYPEFLAG_ENUMINFOS);
    }
}
