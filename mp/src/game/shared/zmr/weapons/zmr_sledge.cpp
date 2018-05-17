#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponSledge C_ZMWeaponSledge
#endif

class CZMWeaponSledge : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponSledge, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

    CZMWeaponSledge();


    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;


    virtual bool CanSecondaryAttack() const OVERRIDE { return true; }
    virtual WeaponSound_t GetSecondaryAttackSound() const { return SPECIAL1; }

    virtual bool UsesAnimEvent( bool bSecondary ) const OVERRIDE { return true; }


    virtual float GetRange() const OVERRIDE { return 60.0f; }
    virtual float GetFireRate() OVERRIDE { return 2.9f; }
    virtual float GetDamageForActivity( Activity act ) const OVERRIDE
    {
        // ZMRTODO: Stop using random values.
        float damage = 50.0f;

        if ( act == ACT_VM_HITCENTER2 )
        {
            damage *= random->RandomFloat( 0.99f, 2.5f );
        }
        else
        {
            damage *= random->RandomFloat( 0.55f, 1.1f );
        }

        return damage;
    };

    virtual void Hit( trace_t& traceHit, Activity iHitActivity ) OVERRIDE;


    virtual void AddViewKick() OVERRIDE
    {
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( !pPlayer ) return;


        QAngle ang;
        ang.x = SharedRandomFloat( "sledgex", 5.0f, 10.0f );
        ang.y = SharedRandomFloat( "sledgey", -2.0f, -1.0f );
        ang.z = 0.0f;

	    pPlayer->ViewPunch( ang );
    }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponSledge, DT_ZM_WeaponSledge )


// We use animation events, so we need to network the attack time.
BEGIN_NETWORK_TABLE( CZMWeaponSledge, DT_ZM_WeaponSledge )
#ifdef CLIENT_DLL
    RecvPropTime( RECVINFO( m_flAttackHitTime ) ),
#else
    SendPropTime( SENDINFO( m_flAttackHitTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponSledge )
    DEFINE_PRED_FIELD_TOL( m_flAttackHitTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_sledge, CZMWeaponSledge );
PRECACHE_WEAPON_REGISTER( weapon_zm_sledge );

acttable_t CZMWeaponSledge::m_acttable[] = 
{
    /*
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,              true },
    { ACT_HL2MP_IDLE,					ACT_ZM_IDLE_SLEDGE,					false },
    { ACT_HL2MP_RUN,					ACT_ZM_RUN_SLEDGE,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_ZM_IDLE_CROUCH_SLEDGE,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_ZM_WALK_CROUCH_SLEDGE,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,	    false },
    { ACT_HL2MP_JUMP,					ACT_ZM_JUMP_SLEDGE,					false },
    */
    { ACT_MP_STAND_IDLE,				ACT_ZM_IDLE_SLEDGE,                 false },
    { ACT_MP_CROUCH_IDLE,				ACT_ZM_IDLE_CROUCH_SLEDGE,			false },
    { ACT_MP_RUN,					    ACT_ZM_RUN_SLEDGE,					false },
    { ACT_MP_CROUCHWALK,			    ACT_ZM_WALK_CROUCH_SLEDGE,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_ZM_GESTURE_RANGE_ATTACK_SLEDGE,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_MELEE,     false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,     false },
    { ACT_MP_JUMP,					    ACT_ZM_JUMP_SLEDGE,                 false },
};
IMPLEMENT_ACTTABLE( CZMWeaponSledge );

CZMWeaponSledge::CZMWeaponSledge()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
}

void CZMWeaponSledge::Hit( trace_t& traceHit, Activity iHitActivity )
{
    BaseClass::Hit( traceHit, iHitActivity );


    AddViewKick();

#ifndef CLIENT_DLL
    PlayAISound();
#endif
}

void CZMWeaponSledge::PrimaryAttack()
{
    if ( !CanPrimaryAttack() )
        return;


    Swing( false );


    // Setup our next attack times
    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
    m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 1.1f;
}

void CZMWeaponSledge::SecondaryAttack()
{
    if ( !CanSecondaryAttack() )
        return;

        
    Swing( true );


    // Setup our next attack times
    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate() * 1.5f;
    m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 1.8f;
}
