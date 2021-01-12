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
    DECLARE_ACTTABLE();

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

acttable_t	CZMWeaponR700::m_acttable[] = 
{
    /*
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_AR2,                 false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,          false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_AR2,                  false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_AR2,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_AR2,       false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_AR2,       false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_AR2,                 false },
};
IMPLEMENT_ACTTABLE( CZMWeaponR700 );


CZMWeaponR700::CZMWeaponR700()
{
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_R700 );
}
