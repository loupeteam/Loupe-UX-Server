///////////////////////////////////////////////////////////////////////////////
// This is a part of the Beckhoff TwinCAT ADS API
// Copyright (C) Beckhoff Automation GmbH
// All rights reserved.
////////////////////////////////////////////////////////////////////////////////


#ifndef __ADSDEF_2_H__
#define __ADSDEF_2_H__

#include "AdsDef.h"

#ifndef WIN32
	typedef char BYTE;
	typedef uint32_t ULONG;
	typedef uint16_t UINT16;
	typedef uint32_t UINT;
	typedef int32_t BOOL;
	typedef char* PCHAR;
	typedef BYTE* PBYTE;
	typedef ULONG* PULONG;
	typedef void * PVOID;
	typedef uint16_t USHORT;
	#define FALSE 0
	#define TRUE 1
#endif



#ifndef	ANYSIZE_ARRAY
#define	ANYSIZE_ARRAY					1
#endif

#define	ADS_FIXEDNAMESIZE				16

#pragma	pack( push, 1)

#define	ADSNOTIFICATION_PDATA( pAdsNotificationHeader )	\
	(	(unsigned char*)	(((PAdsNotificationHeader)pAdsNotificationHeader->data )

////////////////////////////////////////////////////////////////////////////////
// ADS data types
typedef uint16_t			ADS_UINT16;
typedef uint32_t			ADS_UINT32;

////////////////////////////////////////////////////////////////////////////////
// ADS symbol information
typedef AdsSymbolEntry *PAdsSymbolEntry, **PPAdsSymbolEntry;


#define	PADSSYMBOLNAME(p)			((char*)(((PAdsSymbolEntry)p)+1))
#define	PADSSYMBOLTYPE(p)			(((char*)(((PAdsSymbolEntry)p)+1))+((PAdsSymbolEntry)p)->nameLength+1)
#define	PADSSYMBOLCOMMENT(p)		(((char*)(((PAdsSymbolEntry)p)+1))+((PAdsSymbolEntry)p)->nameLength+1+((PAdsSymbolEntry)p)->typeLength+1)

#define	PADSNEXTSYMBOLENTRY(pEntry)	(*((unsigned long*)(((char*)pEntry)+((PAdsSymbolEntry)pEntry)->entryLength)) \
						? ((PAdsSymbolEntry)(((char*)pEntry)+((PAdsSymbolEntry)pEntry)->entryLength)): NULL)



////////////////////////////////////////////////////////////////////////////////
#define	ADSDATATYPEFLAG_DATATYPE		0x00000001
#define	ADSDATATYPEFLAG_DATAITEM		0x00000002

#define	ADSDATATYPE_VERSION_NEWEST		0x00000001

typedef struct
{
	uint32_t		lBound;
	uint32_t		elements;
} AdsDatatypeArrayInfo, *PAdsDatatypeArrayInfo;

typedef struct
{
	ADS_UINT32		entryLength;	// length of complete datatype entry
	ADS_UINT32		version;			// version of datatype structure
	union {
	ADS_UINT32		hashValue;		// hashValue of datatype to compare datatypes
	ADS_UINT32		offsGetCode;	// code offset to getter method
	};
	union {
	ADS_UINT32		typeHashValue;	// hashValue of base type
	ADS_UINT32		offsSetCode;	// code offset to setter method
	};
	ADS_UINT32		size;			// size of datatype ( in bytes )
	ADS_UINT32		offs;			// offs of dataitem in parent datatype ( in bytes )
	ADS_UINT32		dataType;		// adsDataType of symbol (if alias)
	ADS_UINT32		flags;			//
	ADS_UINT16		nameLength;		// length of datatype name (excl. \0)
	ADS_UINT16		typeLength;		// length of dataitem type name (excl. \0)
	ADS_UINT16		commentLength;	// length of comment (excl. \0)
	ADS_UINT16		arrayDim;		//
	ADS_UINT16		subItems;		//
	// ADS_INT8		name[];			// name of datatype with terminating \0
	// ADS_INT8		type[];			// type name of dataitem with terminating \0
	// ADS_INT8		comment[];		// comment of datatype with terminating \0
	// AdsDatatypeArrayInfo	array[];
	// AdsDatatypeEntry		subItems[];
	// GUID			typeGuid;		// typeGuid of this type if ADSDATATYPEFLAG_TYPEGUID is set
	// ADS_UINT8	copyMask[];		// "size" bytes containing 0xff or 0x00 - 0x00 means ignore byte (ADSIGRP_SYM_VALBYHND_WITHMASK)
} AdsDatatypeEntry, *PAdsDatatypeEntry, **PPAdsDatatypeEntry;

#define	PADSDATATYPENAME(p)			((PCHAR)(((PAdsDatatypeEntry)p)+1))
#define	PADSDATATYPETYPE(p)			(((PCHAR)(((PAdsDatatypeEntry)p)+1))+((PAdsDatatypeEntry)p)->nameLength+1)
#define	PADSDATATYPECOMMENT(p)		(((PCHAR)(((PAdsDatatypeEntry)p)+1))+((PAdsDatatypeEntry)p)->nameLength+1+((PAdsDatatypeEntry)p)->typeLength+1)
#define	PADSDATATYPEARRAYINFO(p)	(PAdsDatatypeArrayInfo)(((PCHAR)(((PAdsDatatypeEntry)p)+1))+((PAdsDatatypeEntry)p)->nameLength+1+((PAdsDatatypeEntry)p)->typeLength+1+((PAdsDatatypeEntry)p)->commentLength+1)

__inline PAdsDatatypeEntry AdsDatatypeStructItem(PAdsDatatypeEntry p, unsigned short iItem)
{
	unsigned	short i;
	PAdsDatatypeEntry	pItem;
	if ( iItem >= p->subItems )
		return NULL;
	pItem = (PAdsDatatypeEntry)(((unsigned char*)(p+1))+p->nameLength+p->typeLength+p->commentLength+3+p->arrayDim*sizeof(AdsDatatypeArrayInfo));
	for ( i=0; i < iItem; i++ )
		pItem = (PAdsDatatypeEntry)(((unsigned char*)pItem)+pItem->entryLength);
	return pItem;
}

typedef struct
{
	uint32_t		nSymbols;
	uint32_t		nSymSize;
	uint32_t		nDatatypes;
	uint32_t		nDatatypeSize;
	uint32_t		nMaxDynSymbols;
	uint32_t		nUsedDynSymbols;
} AdsSymbolUploadInfo2, *PAdsSymbolUploadInfo2;

#pragma	pack( pop )

#endif	// __ADSDEF_H__
