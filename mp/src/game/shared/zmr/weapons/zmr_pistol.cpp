#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr_player_shared.h"


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
