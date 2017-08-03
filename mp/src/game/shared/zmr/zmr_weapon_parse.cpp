#include "cbase.h"
#include <KeyValues.h>
#include "ammodef.h"

#include "zmr_weapon_parse.h"


FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CZMWeaponInfo;
}

CZMWeaponInfo::CZMWeaponInfo()
{
	m_iPlayerDamage = 0;
}

void CZMWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );
}