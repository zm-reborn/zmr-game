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
    DECLARE_ACTTABLE();

    CZMWeaponShotgun();
    

    virtual Activity GetReloadStartAct() OVERRIDE { return ACT_SHOTGUN_RELOAD_START; }
    virtual Activity GetReloadEndAct() OVERRIDE { return ACT_SHOTGUN_RELOAD_FINISH; }
    virtual Activity GetPumpAct() OVERRIDE { return ACT_SHOTGUN_PUMP; }
};

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

acttable_t CZMWeaponShotgun::m_acttable[] = 
{
    /*
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SHOTGUN,                 false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,          false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_SHOTGUN,                  false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_SHOTGUN,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,       false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,       false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_SHOTGUN,                 false },
};
IMPLEMENT_ACTTABLE( CZMWeaponShotgun );

CZMWeaponShotgun::CZMWeaponShotgun()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_SHOTGUN );
}
