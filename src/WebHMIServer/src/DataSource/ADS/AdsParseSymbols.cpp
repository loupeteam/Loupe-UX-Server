// AdsParseSymbols.cpp: implementation of the CAdsParseSymbols class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "adsdatasrc_impl.h"
#include "AdsParseSymbols.h"
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include "util.h"
#define ASSERT( x )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// CAdsParseSymbols


CAdsParseSymbols::CAdsParseSymbols(PVOID pSymbols, UINT nSymSize, PVOID pDatatypes, UINT nDTSize)
{
	memset(m_bufGetTypeByNameBuffer, 0, sizeof(m_bufGetTypeByNameBuffer));
	m_pSymbols = new BYTE[nSymSize+sizeof(ULONG)];
	if ( m_pSymbols )
	{
		m_nSymSize = nSymSize;
		if ( nSymSize )
			memcpy(m_pSymbols, pSymbols, nSymSize);
		*(PULONG)&m_pSymbols[nSymSize] = 0;
		m_nSymbols = 0;
		UINT offs = 0;
		while (*(PULONG)&m_pSymbols[offs])
		{
			m_nSymbols++;
			offs += *(PULONG)&m_pSymbols[offs];
		}
		ASSERT(offs==nSymSize);
		m_ppSymbolArray = new PAdsSymbolEntry[m_nSymbols];
		m_nSymbols = offs = 0;
		while (*(PULONG)&m_pSymbols[offs])
		{
			m_ppSymbolArray[m_nSymbols++] = (PAdsSymbolEntry)&m_pSymbols[offs];
			offs += *(PULONG)&m_pSymbols[offs];
		}
		ASSERT(offs==nSymSize);
	}

	m_pDatatypes = new BYTE[nDTSize+sizeof(ULONG)];
	if ( m_pDatatypes )
	{
		m_nDTSize = nDTSize;
		if ( nDTSize )
			memcpy(m_pDatatypes, pDatatypes, nDTSize);
		*(PULONG)&m_pDatatypes[nDTSize] = 0;
		m_nDatatypes = 0;
		UINT offs = 0;
		while (*(PULONG)&m_pDatatypes[offs])
		{
			m_nDatatypes++;
			offs += *(PULONG)&m_pDatatypes[offs];
		}
		ASSERT(offs==nDTSize);
		m_ppDatatypeArray = new PAdsDatatypeEntry[m_nDatatypes];
		m_nDatatypes = offs = 0;
		while (*(PULONG)&m_pDatatypes[offs])
		{
			m_ppDatatypeArray[m_nDatatypes++] = (PAdsDatatypeEntry)&m_pDatatypes[offs];
			offs += *(PULONG)&m_pDatatypes[offs];
		}
		ASSERT(offs==nDTSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
CAdsParseSymbols::~CAdsParseSymbols()
{
	if ( m_pSymbols )
		delete m_pSymbols;
	if ( m_pDatatypes )
		delete m_pDatatypes;
	if ( m_ppSymbolArray )
		delete m_ppSymbolArray;
	if ( m_ppDatatypeArray )
		delete m_ppDatatypeArray;
}

static int _cdecl CompareDTByName( const void* p1, const void* p2 )
{
	return strcmp( (char*)((*(PPAdsDatatypeEntry)p1)+1), (char*)((*(PPAdsDatatypeEntry)p2)+1) );
}

PAdsDatatypeEntry	CAdsParseSymbols::GetTypeByName(std::string sType)
{
	PAdsDatatypeEntry pKey = (PAdsDatatypeEntry)m_bufGetTypeByNameBuffer;
	strcpy((PCHAR)(pKey+1), sType.c_str());

	// serach data type by name
	PPAdsDatatypeEntry	ppEntry = (PPAdsDatatypeEntry)bsearch(&pKey, m_ppDatatypeArray, m_nDatatypes, 
		sizeof(*m_ppDatatypeArray), CompareDTByName);

	if ( ppEntry )
		return *ppEntry;
	else
		return NULL;
}

PAdsDatatypeEntry	CAdsParseSymbols::GetTypeByIndex(UINT sym)
{
	if( sym < m_nDatatypes )
		return m_ppDatatypeArray[sym];
	else
		return NULL;
}

UINT	CAdsParseSymbols::SubSymbolCount(UINT sym)
{
	if ( sym < m_nSymbols )
		return SubSymbolCount(SymbolType(sym));
	else
		return 0;
}

UINT	CAdsParseSymbols::SubSymbolCount(PCHAR sType)
{
	PAdsDatatypeEntry	pEntry = GetTypeByName(sType);
	if ( pEntry )
		return SubSymbolCount(pEntry);
	else
		return 0;
}

UINT	CAdsParseSymbols::SubSymbolCount(CAdsSymbolInfo	&Symbol){
	if ( !Symbol.m_pEntry )
		Symbol.m_pEntry = GetTypeByName(Symbol.type);

	return SubSymbolCount(Symbol.m_pEntry);
}

UINT	CAdsParseSymbols::SubSymbolCount(PAdsDatatypeEntry	pEntry)
{
	UINT cnt=0;
	if ( pEntry )
	{
		if ( pEntry->subItems )
		{
			cnt += pEntry->subItems;
		}
		else if ( pEntry->arrayDim )
		{
			cnt = 1;
			PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
			for ( USHORT i=0; i < pEntry->arrayDim; i++ )
				cnt *= pAI[i].elements;
		}
	}

	return cnt;
}

BOOL	CAdsParseSymbols::Symbol(UINT sym, CAdsSymbolInfo& info)
{
	PAdsSymbolEntry pEntry = Symbol(sym);
	if ( pEntry == NULL )
		return FALSE;
	info.iGrp		= pEntry->iGroup;
	info.iOffs		= pEntry->iOffs;
	info.size		= pEntry->size;
	info.dataType	= pEntry->dataType;
	info.flags		= pEntry->flags;
	info.name		= PADSSYMBOLNAME(pEntry);
	info.fullname	= PADSSYMBOLNAME(pEntry);
	info.type		= PADSSYMBOLTYPE(pEntry);
	info.comment	= PADSSYMBOLCOMMENT(pEntry);
	return TRUE;
}

BOOL	CAdsParseSymbols::Symbol( std::string name, CAdsSymbolInfo& info )
{
	int i=0;
	 PAdsSymbolEntry pEntry = NULL;
	toLower(name);
	while( pEntry = Symbol(i++) ){
		std::string symbolName = PADSSYMBOLNAME(pEntry);
		toLower(symbolName);
		if( name._Starts_with(symbolName) ){
			break;
		}
	}
	if ( pEntry == NULL )
		return FALSE;
	info.iGrp		= pEntry->iGroup;
	info.iOffs		= pEntry->iOffs;
	info.size		= pEntry->size;
	info.dataType	= pEntry->dataType;
	info.flags		= pEntry->flags;
	info.name		= PADSSYMBOLNAME(pEntry);
	info.fullname	= PADSSYMBOLNAME(pEntry);
	info.type		= PADSSYMBOLTYPE(pEntry);
	info.comment	= PADSSYMBOLCOMMENT(pEntry);
	return TRUE;
}

BOOL	CAdsParseSymbols::SubSymbolInfo(CAdsSymbolInfo &main, UINT sub, CAdsSymbolInfo& info)
{
	if ( !main.m_pEntry )
		main.m_pEntry = GetTypeByName(main.type);
	PAdsDatatypeEntry pEntry = main.m_pEntry;
	if ( pEntry )
	{
		if ( pEntry->subItems )
		{
			PAdsDatatypeEntry	pSEntry = AdsDatatypeStructItem(pEntry, sub);
			if ( pSEntry )
			{
				info.iGrp		= main.iGrp;
				info.iOffs		= main.iOffs + pSEntry->offs;
				info.size		= pSEntry->size;
				info.dataType	= pSEntry->dataType;
				info.flags		= pSEntry->flags;
				info.name		= PADSDATATYPENAME(pSEntry);
				info.fullname   = main.fullname + "." + info.name;
				info.type		= PADSDATATYPETYPE(pSEntry);
				info.comment	= PADSDATATYPECOMMENT(pSEntry);
				return TRUE;
			}
		}
		else if ( pEntry->arrayDim )
		{
			UINT x[10]={0}, baseSize=pEntry->size;
			x[pEntry->arrayDim] = 1;
			PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
			for ( int i=pEntry->arrayDim-1; i >= 0 ; i-- )
			{
				x[i] = x[i+1]*pAI[i].elements;
				if ( pAI[i].elements )
					baseSize /= pAI[i].elements;
			}
			if ( sub == 0 && x[0] > 1000 )
			{
				std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
			}
			if ( sub < x[0] )
			{
				info.iGrp		= main.iGrp;
				info.iOffs		= main.iOffs;
				info.size		= baseSize;
				info.dataType	= pEntry->dataType;
				info.flags		= pEntry->flags;
				info.type		= PADSDATATYPETYPE(pEntry);
				info.comment	= PADSDATATYPECOMMENT(pEntry);
				std::string arr ="[";
				for ( int i = 0; i < pEntry->arrayDim; i++ )
				{
					arr	+= pAI[i].lBound + sub / x[i+1];
					arr	+= (i==pEntry->arrayDim-1) ? ']' : ',';
				}
				info.name		= main.name + arr;
				info.fullname	= main.fullname + arr;	
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL	CAdsParseSymbols::SubSymbolEntry(CAdsSymbolInfo &main, UINT sub, AdsDatatypeEntry &SEntry){
		if ( !main.m_pEntry )
			main.m_pEntry = GetTypeByName(main.type);
		return SubSymbolEntry(main.m_pEntry, sub, SEntry);
}

BOOL	CAdsParseSymbols::SubSymbolEntry(PAdsDatatypeEntry pEntry, UINT sub, AdsDatatypeEntry &SEntry)
{
	if ( pEntry )
	{
		if ( pEntry->subItems )
		{
			SEntry = *AdsDatatypeStructItem(pEntry, sub);
		}
		else if ( pEntry->arrayDim )
		{
			UINT x[10]={0}, baseSize=pEntry->size;
			x[pEntry->arrayDim] = 1;
			PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
			for ( int i=pEntry->arrayDim-1; i >= 0 ; i-- )
			{
				x[i] = x[i+1]*pAI[i].elements;
				if ( pAI[i].elements )
					baseSize /= pAI[i].elements;
			}
			if ( sub == 0 && x[0] > 1000 )
			{
				std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
			}
			if ( sub < x[0] )
			{
				SEntry = *pEntry;
				std::string arr ="[";
				for ( int i = 0; i < pEntry->arrayDim; i++ )
				{
//					arr	+= string::format("%d", pAI[i].lBound + sub / x[i+1]);
//					arr	+= (i==pEntry->arrayDim-1) ? ']' : ',';
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL	CAdsParseSymbols::SubSymbolInfo(PAdsDatatypeEntry Entry, UINT sub, CAdsSymbolInfo& info)
{
	PAdsDatatypeEntry pEntry = Entry;
	if ( pEntry )
	{
		if ( pEntry->subItems )
		{
			PAdsDatatypeEntry	pSEntry = AdsDatatypeStructItem(pEntry, sub);
			if ( pSEntry )
			{
				info.iGrp		= 0;
				info.iOffs		= pSEntry->offs;
				info.size		= pSEntry->size;
				info.dataType	= pSEntry->dataType;
				info.flags		= pSEntry->flags;
				info.name		= PADSDATATYPENAME(pSEntry);
				info.fullname   = info.name;
				info.type		= PADSDATATYPETYPE(pSEntry);
				info.comment	= PADSDATATYPECOMMENT(pSEntry);
				return TRUE;
			}
		}
		else if ( pEntry->arrayDim )
		{
			UINT x[10]={0}, baseSize=pEntry->size;
			x[pEntry->arrayDim] = 1;
			PAdsDatatypeArrayInfo pAI = PADSDATATYPEARRAYINFO(pEntry);
			for ( int i=pEntry->arrayDim-1; i >= 0 ; i-- )
			{
				x[i] = x[i+1]*pAI[i].elements;
				if ( pAI[i].elements )
					baseSize /= pAI[i].elements;
			}
			if ( sub == 0 && x[0] > 1000 )
			{
				std::cout << "Warning: array size is " << x[0] << " elements, this may take a while to parse.\n";
			}
			if ( sub < x[0] )
			{
				info.iGrp		= 0;
				info.iOffs		= 0;
				info.size		= baseSize;
				info.dataType	= pEntry->dataType;
				info.flags		= pEntry->flags;
				info.type		= PADSDATATYPETYPE(pEntry);
				info.comment	= PADSDATATYPECOMMENT(pEntry);
/* TODO
				CString arr='[', tmp;
				for ( int i = 0; i < pEntry->arrayDim; i++ )
				{
					tmp.Format(_T("%d"), pAI[i].lBound + sub / x[i+1]);
					arr	+= tmp;
					arr	+= (i==pEntry->arrayDim-1) ? ']' : ',';
					info.iOffs	+= baseSize * x[i+1] * (sub/x[i+1]);
					sub %= x[i+1];
				}
*/				
				info.name		= std::string(PADSSYMBOLNAME(pEntry)) + "[" + std::to_string(sub) + "]";
				info.fullname	= std::string(PADSSYMBOLNAME(pEntry)) + info.name;
				return TRUE;
			}
		}
	}
	return FALSE;
}

