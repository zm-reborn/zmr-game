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
	m_flDamage = 0.0f;
    m_bUseHands = false;
}

void CZMWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

    m_flDamage = pKeyValuesData->GetFloat( "damage", 0.0f );
    m_bUseHands = pKeyValuesData->GetBool( "usenewhands", false );
}
