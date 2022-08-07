#include "cbase.h"

#include "zmr_shareddefs.h"
#include "zmr_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
#define CZMWeaponPistolGlock C_ZMWeaponPistolGlock
#endif

class CZMWeaponPistolGlock : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponPistolGlock, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponPistolGlock();
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponPistolGlock, DT_ZM_WeaponPistolGlock )

BEGIN_NETWORK_TABLE( CZMWeaponPistolGlock, DT_ZM_WeaponPistolGlock )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponPistolGlock )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_glock, CZMWeaponPistolGlock );
PRECACHE_WEAPON_REGISTER( weapon_zm_glock );


CZMWeaponPistolGlock::CZMWeaponPistolGlock()
{
    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_PISTOL_GLOCK );
}
