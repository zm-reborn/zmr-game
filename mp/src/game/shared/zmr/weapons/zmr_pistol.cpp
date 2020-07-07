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
	DECLARE_ACTTABLE();

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

acttable_t CZMWeaponPistol::m_acttable[] = 
{
    /*
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_PISTOL,                  false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_PISTOL,					false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,  false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_PISTOL,                  false },
};
IMPLEMENT_ACTTABLE( CZMWeaponPistol );

CZMWeaponPistol::CZMWeaponPistol()
{
    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_PISTOL );
}
