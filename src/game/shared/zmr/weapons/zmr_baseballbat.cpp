#include "cbase.h"

#include "in_buttons.h"


#include "zmr_shareddefs.h"
#include "zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponBaseballBat C_ZMWeaponBaseballBat
#endif

class CZMWeaponBaseballBat : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponBaseballBat, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponBaseballBat();


    virtual bool UsesAnimEvent( bool bSecondary ) const override { return false; }

    virtual bool CanSecondaryAttack() const override { return false; }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponBaseballBat, DT_ZM_WeaponBaseballBat )

BEGIN_NETWORK_TABLE( CZMWeaponBaseballBat, DT_ZM_WeaponBaseballBat )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponBaseballBat )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_baseballbat, CZMWeaponBaseballBat );
PRECACHE_WEAPON_REGISTER( weapon_zm_baseballbat );


CZMWeaponBaseballBat::CZMWeaponBaseballBat()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_BASEBALLBAT );
}
