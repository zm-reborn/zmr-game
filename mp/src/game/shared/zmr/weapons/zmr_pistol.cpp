#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr/zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr/zmr_player_shared.h"


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


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_ammo_pistol"; };
    int GetDropAmmoAmount() OVERRIDE { return SIZE_AMMO_PISTOL; };
#endif

    virtual const Vector& GetBulletSpread( void ) OVERRIDE
    {
        static Vector cone = VECTOR_CONE_2DEGREES;
        return cone;
    }
    
    virtual void AddViewKick( void ) OVERRIDE
    {
        CZMPlayer* pPlayer = ToZMPlayer( GetOwner() );

        if ( !pPlayer ) return;


	    QAngle	viewPunch;

	    viewPunch.x = SharedRandomFloat( "pistolpax", 0.25f, 0.5f );
	    viewPunch.y = SharedRandomFloat( "pistolpay", -.6f, .6f );
	    viewPunch.z = 0.0f;

	    //Add it to the view punch
	    pPlayer->ViewPunch( viewPunch );
    }
    
    virtual float GetFireRate( void ) OVERRIDE { return 0.375f; };
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
    m_fMinRange1 = m_fMinRange2 = 24.0f;
    m_fMaxRange1 = m_fMaxRange2 = 1500.0f;

    m_bFiresUnderwater = true;

    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
}
