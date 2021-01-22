#include "cbase.h"

#include "zmr_rifle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
#define CZMWeaponR700 C_ZMWeaponR700
#endif

class CZMWeaponR700 : public CZMWeaponRifle
{
public:
    DECLARE_CLASS( CZMWeaponR700, CZMWeaponRifle );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CZMWeaponR700();
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponR700, DT_ZM_WeaponR700 )

BEGIN_NETWORK_TABLE( CZMWeaponR700, DT_ZM_WeaponR700 )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponR700 )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_r700, CZMWeaponR700 );
PRECACHE_WEAPON_REGISTER( weapon_zm_r700 );


CZMWeaponR700::CZMWeaponR700()
{
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_R700 );
}
