#pragma once

#include "weapon_parse.h"
#include "networkvar.h"


/*
    NOTE: Remove hl2mp/hl2mp_weapon_parse.cpp
    This makes HL2MP weapons crash, btw so gotta fix dat.

*/

class CZMWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CZMWeaponInfo, FileWeaponInfo_t );
	
	CZMWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );



	float m_flDamage;
};
