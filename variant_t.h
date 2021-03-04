//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VARIANT_T_H
#define VARIANT_T_H
#ifdef _WIN32
#pragma once
#endif


#include "ehandle.h"
#include "mathlib/vmatrix.h"

class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;

//
// A variant class for passing data in entity input/output connections.
//
class variant_t
{
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	CHandle<CBaseEntity> eVal; // this can't be in the union because it has a constructor.

	fieldtype_t fieldType;

public:

	// constructor
	variant_t() : iVal(0), fieldType(FIELD_VOID) {}
	void SetString( string_t str ) { iszVal = str, fieldType = FIELD_STRING; }
	inline string_t StringID( void ) const	{ return( fieldType == FIELD_STRING ) ? iszVal : NULL_STRING; }
};
#endif // VARIANT_T_H
