#include "cbase.h"

#include "in_buttons.h"


#include "zmr_shareddefs.h"
#include "zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponImprovised C_ZMWeaponImprovised
#endif

class CZMWeaponImprovised : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponImprovised, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponImprovised();


    virtual bool UsesAnimEvent( bool bSecondary ) const OVERRIDE { return false; }

    virtual bool CanSecondaryAttack() const OVERRIDE { return false; }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponImprovised, DT_ZM_WeaponImprovised )

BEGIN_NETWORK_TABLE( CZMWeaponImprovised, DT_ZM_WeaponImprovised )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponImprovised )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_improvised, CZMWeaponImprovised );
#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( weapon_crowbar, CZMWeaponImprovised ); // // Some shit maps may spawn weapon_crowbar.
#endif
PRECACHE_WEAPON_REGISTER( weapon_zm_improvised );


CZMWeaponImprovised::CZMWeaponImprovised()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_IMPROVISED );
}
