// AdsParseSymbols.h: interface for the CAdsParseSymbols class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_)
#define AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////////////////////
class CAdsSymbolInfo
{
public:
	CAdsSymbolInfo() : m_pEntry(NULL) {};
	ULONG		iGrp;				// 
	ULONG		iOffs;			// 
	ULONG		size;				// size of datatype ( in bytes )
	ULONG		offs;				// offs of dataitem in parent datatype ( in bytes )
	ULONG		dataType;		// adsDataType of symbol (if alias)
	ULONG		flags;			// 
	std::string	name;
	std::string	fullname;
	std::string	type;
	std::string	comment;
	PAdsDatatypeEntry	m_pEntry;
};

///////////////////////////////////////////////////////////////////////////////
// CAdsParseSymbols
class CAdsParseSymbols
{
public:
	CAdsParseSymbols(PVOID pSymbols, UINT nSymSize, PVOID pDatatypes=NULL, UINT nDTSize=0);
	virtual	~CAdsParseSymbols();

	virtual	UINT	SymbolCount(){ return m_nSymbols; }
	virtual	UINT	DatatypeCount(){ return m_nDatatypes; }

	virtual	PAdsSymbolEntry	Symbol(UINT sym){ return (sym < m_nSymbols) ? m_ppSymbolArray[sym] : NULL; }
	virtual	BOOL	Symbol(UINT sym, CAdsSymbolInfo& info);
	virtual BOOL	Symbol(std::string name, CAdsSymbolInfo& info);

	virtual	PCHAR	SymbolName(UINT sym){ return (sym < m_nSymbols) ? PADSSYMBOLNAME(m_ppSymbolArray[sym]) : NULL; }

	virtual	PCHAR	SymbolType(UINT sym){ return (sym < m_nSymbols) ? PADSSYMBOLTYPE(m_ppSymbolArray[sym]) : NULL; }

	virtual	PCHAR	SymbolComment(UINT sym){ return (sym < m_nSymbols) ? PADSSYMBOLCOMMENT(m_ppSymbolArray[sym]) : NULL; }

	virtual	UINT	SubSymbolCount(UINT sym);
	virtual	UINT	SubSymbolCount(PCHAR sType);
	virtual	UINT	SubSymbolCount(PAdsDatatypeEntry	pEntry);
	virtual UINT	SubSymbolCount(CAdsSymbolInfo	&Symbol);

	virtual	BOOL	SubSymbolInfo(CAdsSymbolInfo &main, UINT sub, CAdsSymbolInfo& info);
	virtual BOOL	SubSymbolEntry(CAdsSymbolInfo &main, UINT sub, AdsDatatypeEntry &SEntry);

	virtual BOOL	SubSymbolInfo(PAdsDatatypeEntry Entry, UINT sub, CAdsSymbolInfo& info);
	virtual BOOL	SubSymbolEntry(PAdsDatatypeEntry Entry, UINT sub, AdsDatatypeEntry &SEntry);

	virtual	PAdsDatatypeEntry	GetTypeByName(std::string sType);
	virtual	PAdsDatatypeEntry	GetTypeByIndex(UINT sym);
protected:	
	PBYTE							m_pSymbols;
	PBYTE							m_pDatatypes;
	UINT							m_nSymbols;
	UINT							m_nDatatypes;
	UINT							m_nSymSize;
	UINT							m_nDTSize;
	PPAdsSymbolEntry				m_ppSymbolArray;
	PPAdsDatatypeEntry				m_ppDatatypeArray;
	BYTE							m_bufGetTypeByNameBuffer[300];
};

#endif // !defined(AFX_ADSPARSESYMBOLS_H__353EFDFE_6136_400C_A0E7_B24C2480E9F1__INCLUDED_)
