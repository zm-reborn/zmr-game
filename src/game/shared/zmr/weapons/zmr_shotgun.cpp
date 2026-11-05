#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr_basepump.h"


#ifdef CLIENT_DLL
#define CZMWeaponShotgun C_ZMWeaponShotgun
#endif

class CZMWeaponShotgun : public CZMBasePumpWeapon
{
public:
    DECLARE_CLASS( CZMWeaponShotgun, CZMBasePumpWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CZMWeaponShotgun();
    

    virtual Activity GetReloadStartAct() const override;
    virtual Activity GetReloadEndAct() const override;
    virtual Activity GetPumpAct() const override { return ACT_SHOTGUN_PUMP; }
};

REGISTER_WEAPON_CONFIG( ZMWeaponConfig::ZMCONFIGSLOT_SHOTGUN, CZMBasePumpConfig );

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponShotgun, DT_ZM_WeaponShotgun )

BEGIN_NETWORK_TABLE( CZMWeaponShotgun, DT_ZM_WeaponShotgun )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponShotgun )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_shotgun, CZMWeaponShotgun );
#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( weapon_shotgun, CZMWeaponShotgun ); // Some shit maps may spawn weapon_shotgun.
#endif
PRECACHE_WEAPON_REGISTER( weapon_zm_shotgun );

CZMWeaponShotgun::CZMWeaponShotgun()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_SHOTGUN );
}

Activity CZMWeaponShotgun::GetReloadStartAct() const
{
    return GetActivityOrFallback( ACT_SHOTGUN_RELOAD_START, ACT_VM_RELOAD_START );
}

Activity CZMWeaponShotgun::GetReloadEndAct() const
{
    return GetActivityOrFallback( ACT_SHOTGUN_RELOAD_FINISH, ACT_VM_RELOAD_FINISH );
}
