#include "cbase.h"

#include "zmr_shareddefs.h"
#include "zmr_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
#define CZMWeaponPistol C_ZMWeaponPistol
#endif

class CZMWeaponPistol : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponPistol, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponPistol();
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponPistol, DT_ZM_WeaponPistol )

BEGIN_NETWORK_TABLE( CZMWeaponPistol, DT_ZM_WeaponPistol )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponPistol )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_pistol, CZMWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_zm_pistol );


CZMWeaponPistol::CZMWeaponPistol()
{
    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_PISTOL );
}
