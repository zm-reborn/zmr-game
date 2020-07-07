#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"


#include "zmr_shareddefs.h"
#include "zmr_player_shared.h"

#include "zmr_weaponconfig.h"
#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponSledge C_ZMWeaponSledge
#endif


using namespace ZMWeaponConfig;

class CZMSledgeConfig : public CZMBaseWeaponConfig
{
public:
    CZMSledgeConfig( const char* wepname, const char* configpath ) : CZMBaseWeaponConfig( wepname, configpath )
    {
        flPrimaryRandomMultMin = 1.0f;
        flPrimaryRandomMultMax = 1.0f;

        flSecondaryRandomMultMin = 1.0f;
        flSecondaryRandomMultMax = 1.0f;
    }

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE
    {
        CZMBaseWeaponConfig::LoadFromConfig( kv );

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            flPrimaryRandomMultMin = inner->GetFloat( "sledge_min_mult" );
            flPrimaryRandomMultMax = inner->GetFloat( "sledge_max_mult" );
        }

        inner = kv->FindKey( "SecondaryAttack" );
        if ( inner )
        {
            flSecondaryRandomMultMin = inner->GetFloat( "sledge_min_mult" );
            flSecondaryRandomMultMax = inner->GetFloat( "sledge_max_mult" );
        }
    }

    virtual KeyValues* ToKeyValues() const OVERRIDE
    {
        auto* kv = CZMBaseWeaponConfig::ToKeyValues();

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            inner->SetFloat( "sledge_min_mult", flPrimaryRandomMultMin );
            inner->SetFloat( "sledge_max_mult", flPrimaryRandomMultMax );
        }

        inner = kv->FindKey( "SecondaryAttack" );
        if ( inner )
        {
            inner->SetFloat( "sledge_min_mult", flSecondaryRandomMultMin );
            inner->SetFloat( "sledge_max_mult", flSecondaryRandomMultMax );
        }

        return kv;
    }

    
    float flPrimaryRandomMultMin;
    float flPrimaryRandomMultMax;

    float flSecondaryRandomMultMin;
    float flSecondaryRandomMultMax;
};


REGISTER_WEAPON_CONFIG( ZMCONFIGSLOT_SLEDGE, CZMSledgeConfig );



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


    const CZMSledgeConfig* GetSledgeConfig() const { return static_cast<const CZMSledgeConfig*>( GetWeaponConfig() ); }


    virtual bool CanSecondaryAttack() const OVERRIDE { return true; }
    virtual WeaponSound_t GetSecondaryAttackSound() const { return SPECIAL1; }

    virtual bool UsesAnimEvent( bool bSecondary ) const OVERRIDE { return true; }


    virtual float GetDamageForActivity( Activity act ) const OVERRIDE
    {
        // ZMRTODO: Stop using random values.
        float damage = BaseClass::GetDamageForActivity( act );


        auto* pConfig = GetSledgeConfig();

        if ( act == ACT_VM_HITCENTER2 )
        {
            damage *= random->RandomFloat( pConfig->flSecondaryRandomMultMin, pConfig->flSecondaryRandomMultMax );
        }
        else
        {
            damage *= random->RandomFloat( pConfig->flPrimaryRandomMultMin, pConfig->flPrimaryRandomMultMax );
        }

        return damage;
    };

    virtual void Hit( trace_t& traceHit, Activity iHitActivity ) OVERRIDE;
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponSledge, DT_ZM_WeaponSledge )


BEGIN_NETWORK_TABLE( CZMWeaponSledge, DT_ZM_WeaponSledge )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponSledge )
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
    SetConfigSlot( ZMCONFIGSLOT_SLEDGE );
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
    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

    auto* pMe = GetPlayerOwner();
    if ( pMe )
    {
        pMe->m_flNextAttack = m_flNextPrimaryAttack;
    }
}

void CZMWeaponSledge::SecondaryAttack()
{
    if ( !CanSecondaryAttack() )
        return;

        
    Swing( true );


    // Setup our next attack times
    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

    auto* pMe = GetPlayerOwner();
    if ( pMe )
    {
        pMe->m_flNextAttack = m_flNextPrimaryAttack;
    }
}
