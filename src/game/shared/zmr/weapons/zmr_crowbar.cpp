#include "cbase.h"

#include "in_buttons.h"


#include "zmr_shareddefs.h"
#include "zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponCrowbar C_ZMWeaponCrowbar
#endif

class CZMWeaponCrowbar : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponCrowbar, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponCrowbar();


    virtual bool UsesAnimEvent( bool bSecondary ) const override { return false; }

    virtual bool CanSecondaryAttack() const override { return false; }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponCrowbar, DT_ZM_WeaponCrowbar )

BEGIN_NETWORK_TABLE( CZMWeaponCrowbar, DT_ZM_WeaponCrowbar )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponCrowbar )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_improvised, CZMWeaponCrowbar );
#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( weapon_crowbar, CZMWeaponCrowbar ); // // Some poopy maps may spawn weapon_crowbar.
#endif
PRECACHE_WEAPON_REGISTER( weapon_zm_improvised );


CZMWeaponCrowbar::CZMWeaponCrowbar()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_CROWBAR );
}
