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


    RECORD_PREDICTION_SEED;

    float GetRange() OVERRIDE { return 60.0f; };
    float GetFireRate() OVERRIDE { return 2.9f; };
    float GetDamageForActivity( Activity act ) OVERRIDE
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

    void Hit( trace_t&, Activity ) OVERRIDE;
    void Swing( bool bSecondary, const bool bUseAnimationEvent = true ) OVERRIDE;


    void AddViewKick() OVERRIDE
    {
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( !pPlayer ) return;

        // ZMRTODO: This isn't called on client.
        QAngle ang;
        ang.x = GetPredictedRandomFloat( 5.0f, 10.0f );
        ang.y = GetPredictedRandomFloat( -2.0f, -1.0f );
        ang.z = 0.0f;

	    pPlayer->ViewPunch( ang );
    };


#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t*, CBaseCombatCharacter* ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel*, const Vector&, const QAngle&, int event, const char* ) OVERRIDE;
#endif
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
}

void CZMWeaponSledge::Hit( trace_t& traceHit, Activity nHitActivity )
{
    // Override the default activity.
    Activity act = GetActivity();

    BaseClass::Hit( traceHit, act );


    AddViewKick();
}

void CZMWeaponSledge::Swing( bool bSecondary, const bool bUseAnimationEvent )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    SendWeaponAnim( bSecondary ? ACT_VM_HITCENTER2 : ACT_VM_HITCENTER );
    WeaponSound( SPECIAL1 );

    pPlayer->SetAnimation( PLAYER_ATTACK1 );

    if ( bSecondary )
    {
        m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate() * 1.5f;
        m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 1.8f;
    }
    else
    {
        m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
        m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate() * 1.1f;
    }
}

#ifndef CLIENT_DLL
void CZMWeaponSledge::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_MELEEHIT:
		HandleAnimEventMeleeHit();
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#else
bool CZMWeaponSledge::OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options )
{
    if ( event == AE_ZM_MELEEHIT )
    {
        HandleAnimEventMeleeHit();
        return true;
    }

    return false;
}
#endif
