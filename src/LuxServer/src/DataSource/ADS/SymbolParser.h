// AdsParseSymbols.h: interface for the CAdsParseSymbols class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_)
#define AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <TcAdsDef.h>
#include <TcAdsAPI.h>

struct datatype_flags_struct {
    bool DATATYPE;
    bool DATAITEM;
    bool REFERENCETO;
    bool METHODDEREF;
    bool OVERSAMPLE;
    bool BITVALUES;
    bool PROPITEM;
    bool TYPEGUID;
    bool PERSISTENT;
    bool COPYMASK;
    bool TCCOMIFACEPTR;
    bool METHODINFOS;
    bool ATTRIBUTES;
    bool ENUMINFOS;
    BYTE mask;
    datatype_flags_struct()
    {}
    datatype_flags_struct(ULONG flags, bool symbol);
};
///////////////////////////////////////////////////////////////////////////////
class CAdsSymbolInfo {
public:
    CAdsSymbolInfo() : m_pEntry(NULL) {}
    bool valid;
    ULONG iGrp;                     //
    ULONG iOffs;                //
    ULONG size;                     // size of datatype ( in bytes )
    ULONG offs;                     // offs of dataitem in parent datatype ( in bytes )
    ULONG dataType;             // adsDataType of symbol (if alias)
    UINT16 arrayDim;
    ULONG flags;                //
    datatype_flags_struct flags_struct;
    std::string name;
    std::string fullname;
    std::string type;
    std::string comment;
    PAdsDatatypeEntry m_pEntry;
};

///////////////////////////////////////////////////////////////////////////////
// CAdsParseSymbols
class CAdsParseSymbols {
public:
    CAdsParseSymbols(PVOID pSymbols, UINT nSymSize, PVOID pDatatypes = NULL, UINT nDTSize = 0);
    virtual ~CAdsParseSymbols();

    virtual UINT    SymbolCount(){ return m_nSymbols; }
    virtual UINT    DatatypeCount(){ return m_nDatatypes; }

    virtual PAdsSymbolEntry Symbol(UINT sym){ return (sym < m_nSymbols) ? m_ppSymbolArray[sym] : NULL; }
    virtual BOOL    Symbol(UINT sym, CAdsSymbolInfo& info);
    virtual BOOL    Symbol(std::string name, CAdsSymbolInfo& info);

    virtual PCHAR   SymbolName(UINT sym){ return (sym < m_nSymbols) ? PADSSYMBOLNAME(m_ppSymbolArray[sym]) : NULL; }

    virtual PCHAR   SymbolType(UINT sym){ return (sym < m_nSymbols) ? PADSSYMBOLTYPE(m_ppSymbolArray[sym]) : NULL; }

    virtual PCHAR   SymbolComment(UINT sym)
    {
        return (sym < m_nSymbols) ? PADSSYMBOLCOMMENT(m_ppSymbolArray[sym]) : NULL;
    }

    virtual UINT    SubSymbolCount(UINT sym);
    virtual UINT    SubSymbolCount(PCHAR sType);
    virtual UINT    SubSymbolCount(PAdsDatatypeEntry pEntry);
    virtual UINT    SubSymbolCount(CAdsSymbolInfo& Symbol);

    virtual BOOL    SubSymbolInfo(CAdsSymbolInfo& main, UINT sub, CAdsSymbolInfo& info);
    virtual BOOL    SubSymbolEntry(CAdsSymbolInfo& main, UINT sub, AdsDatatypeEntry& SEntry);

    virtual BOOL    SubSymbolInfo(PAdsDatatypeEntry Entry, UINT sub, CAdsSymbolInfo& info);
    virtual BOOL    SubSymbolEntry(PAdsDatatypeEntry Entry, UINT sub, AdsDatatypeEntry& SEntry);

    virtual PAdsDatatypeEntry   GetTypeByName(std::string sType);
    virtual PAdsDatatypeEntry   GetTypeByIndex(UINT sym);

    virtual void DumpSymbols();
    virtual void DumpDatatypes();

protected:
    PBYTE m_pSymbols;
    PBYTE m_pDatatypes;
    UINT m_nSymbols;
    UINT m_nDatatypes;
    UINT m_nSymSize;
    UINT m_nDTSize;
    PPAdsSymbolEntry m_ppSymbolArray;
    PPAdsDatatypeEntry m_ppDatatypeArray;
    BYTE m_bufGetTypeByNameBuffer[300];
};

// #define ADSDATATYPEFLAG_DATATYPE 1
// #define ADSDATATYPEFLAG_DATAITEM 2
#define ADSDATATYPEFLAG_REFERENCETO 4
#define ADSDATATYPEFLAG_METHODDEREF 8
#define ADSDATATYPEFLAG_OVERSAMPLE 16
#define ADSDATATYPEFLAG_BITVALUES 32
#define ADSDATATYPEFLAG_PROPITEM 64
#define ADSDATATYPEFLAG_TYPEGUID 128
#define ADSDATATYPEFLAG_PERSISTENT 256
#define ADSDATATYPEFLAG_COPYMASK 512
#define ADSDATATYPEFLAG_TCCOMIFACEPTR 1024
#define ADSDATATYPEFLAG_METHODINFOS 2048
#define ADSDATATYPEFLAG_ATTRIBUTES 4096
#define ADSDATATYPEFLAG_ENUMINFOS 8192

#endif // !defined(AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_)
