#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponFists C_ZMWeaponFists
#endif

class CZMWeaponFists : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponFists, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

    CZMWeaponFists();


    virtual bool UsesAnimEvent( bool bSecondary ) const OVERRIDE { return true; }

#ifdef CLIENT_DLL
    virtual bool ShouldDrawPickup() OVERRIDE { return false; }
    virtual bool ShouldDraw() OVERRIDE { return false; }
#endif

    virtual float GetRange() const OVERRIDE { return 45.0f; }
    virtual float GetFireRate() OVERRIDE { return 0.65f; }

    virtual void AddViewKick() OVERRIDE
    {
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( !pPlayer ) return;


        QAngle ang;
        ang.x = SharedRandomFloat( "fistsx", -1.0f, 1.0f );
        ang.y = SharedRandomFloat( "fistsy", -1.0f, 1.0f );
        ang.z = 0.0f;

	    pPlayer->ViewPunch( ang );
    }

    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;

    virtual bool CanSecondaryAttack() const { return true; }
    virtual Activity GetPrimaryAttackActivity() OVERRIDE { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity() OVERRIDE { return ACT_VM_HITCENTER; }

    virtual float GetDamageForActivity( Activity act ) const OVERRIDE { return 5.0f; }


    virtual void Hit( trace_t& traceHit, Activity iHitActivity ) OVERRIDE;


#ifndef CLIENT_DLL
    virtual bool CanBeDropped() const OVERRIDE { return false; }
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponFists, DT_ZM_WeaponFists )

// We use animation events, so we need to network the attack time.
BEGIN_NETWORK_TABLE( CZMWeaponFists, DT_ZM_WeaponFists )
#ifdef CLIENT_DLL
    RecvPropTime( RECVINFO( m_flAttackHitTime ) ),
#else
    SendPropTime( SENDINFO( m_flAttackHitTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponFists )
    DEFINE_PRED_FIELD_TOL( m_flAttackHitTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_fists, CZMWeaponFists );
PRECACHE_WEAPON_REGISTER( weapon_zm_fists );

acttable_t	CZMWeaponFists::m_acttable[] = 
{
    /*
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,                  true },
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_MELEE,                   false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_MELEE,					false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_MELEE,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponFists );

CZMWeaponFists::CZMWeaponFists()
{
    SetSlotFlag( ZMWEAPONSLOT_NONE );
}

void CZMWeaponFists::PrimaryAttack()
{
    if ( !CanPrimaryAttack() )
        return;


    Swing( false );


    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
    m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
}

void CZMWeaponFists::SecondaryAttack()
{
    if ( !CanSecondaryAttack() )
        return;

        
    Swing( true );


    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
    m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
}

void CZMWeaponFists::Hit( trace_t& traceHit, Activity iHitActivity )
{
    BaseClass::Hit( traceHit, iHitActivity );


    AddViewKick();

#ifndef CLIENT_DLL
    PlayAISound();
#endif
}
