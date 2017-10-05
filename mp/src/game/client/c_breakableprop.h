//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_BREAKABLEPROP_H
#define C_BREAKABLEPROP_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef ZMR // ZMRCHANGE: Let the props glow!
#include "glowstencil/c_glowbase.h"

class C_BreakableProp : public C_BaseGlowProp
{
	typedef C_BaseGlowProp BaseClass;
#else
class C_BreakableProp : public C_BaseAnimating
{
	typedef C_BaseAnimating BaseClass;
#endif
public:
	DECLARE_CLIENTCLASS();

	C_BreakableProp();
	
	virtual void SetFadeMinMax( float fademin, float fademax );

	// Copy fade from another breakable prop
	void CopyFadeFrom( C_BreakableProp *pSource );
};

#endif // C_BREAKABLEPROP_H
