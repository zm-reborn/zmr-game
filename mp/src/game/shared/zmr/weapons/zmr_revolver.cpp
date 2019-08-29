#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"
#ifdef CLIENT_DLL
#include "prediction.h"
#endif

#include "in_buttons.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr/zmr_player_shared.h"


#ifdef CLIENT_DLL
#define CZMWeaponRevolver C_ZMWeaponRevolver
#endif

class CZMWeaponRevolver : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponRevolver, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

    CZMWeaponRevolver();


    virtual bool Deploy() OVERRIDE;


    virtual Vector GetBulletSpread() const OVERRIDE
    {
        Vector cone = BaseClass::GetBulletSpread();

        CZMPlayer* pOwner = GetPlayerOwner();

        if ( pOwner )
        {
            float ratio = 1.0f - pOwner->GetAccuracyRatio();
            ratio *= ratio;
            
            
            // 2 degrees
#define     SECONDARY_MIN_RATIO         0.1f

            const bool bIsSecondary = IsInSecondaryAttack();

            // Secondary is not perfectly accurate.
            if ( bIsSecondary )
                ratio = MAX( SECONDARY_MIN_RATIO, ratio );



            cone.x = ratio * cone.x;
            cone.y = ratio * cone.y;
            cone.z = ratio * cone.z;
        }

        return cone;
    }

    
    virtual void AddViewKick() OVERRIDE;

    virtual void PrimaryAttack() OVERRIDE;
    virtual void PrimaryAttackEffects( WeaponSound_t wpnsound ) OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;

    void ItemPostFrame() OVERRIDE;

    void HandleAnimEventRevolver();

#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options ) OVERRIDE;
#endif

protected:
    CNetworkVar( float, m_flShootTime );
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponRevolver, DT_ZM_WeaponRevolver )

BEGIN_NETWORK_TABLE( CZMWeaponRevolver, DT_ZM_WeaponRevolver )
#ifdef CLIENT_DLL
    RecvPropTime( RECVINFO( m_flShootTime ) ),
#else
    SendPropTime( SENDINFO( m_flShootTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponRevolver )
    DEFINE_PRED_FIELD_TOL( m_flShootTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_revolver, CZMWeaponRevolver );
PRECACHE_WEAPON_REGISTER( weapon_zm_revolver );

acttable_t CZMWeaponRevolver::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CZMWeaponRevolver );

CZMWeaponRevolver::CZMWeaponRevolver()
{
    m_flShootTime = 0.0f;


    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 1500.0f;


    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_REVOLVER );
}

bool CZMWeaponRevolver::Deploy()
{
    bool res = BaseClass::Deploy();

    if ( res )
    {
        m_flShootTime = 0.0f;
    }

    return res;
}

void CZMWeaponRevolver::PrimaryAttack()
{
    if ( !CanAct( WEPACTION_ATTACK ) )
        return;

	if ( UsesClipsForAmmo1() && !Clip1() ) 
	{
		Reload();
		return;
	}



    SendWeaponAnim( ACT_VM_PRIMARYATTACK );

    float waittime = GetFirstInstanceOfAnimEventTime( GetSequence(), AE_ZM_REVOLVERSHOOT );
    if ( waittime >= 0.0f )
        m_flShootTime = gpGlobals->curtime + waittime;


    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

void CZMWeaponRevolver::SecondaryAttack()
{
    if ( !CanAct( WEPACTION_ATTACK2 ) )
        return;

	if ( UsesClipsForAmmo1() && !Clip1() ) 
	{
		Reload();
		return;
	}


    SendWeaponAnim( ACT_VM_SECONDARYATTACK );

    float waittime = GetFirstInstanceOfAnimEventTime( GetSequence(), AE_ZM_REVOLVERSHOOT );
    if ( waittime >= 0.0f )
        m_flShootTime = gpGlobals->curtime + waittime;


    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

void CZMWeaponRevolver::ItemPostFrame()
{
    BaseClass::ItemPostFrame();


    if ( m_flShootTime != 0.0f && m_flShootTime <= gpGlobals->curtime )
    {
        Shoot();

        m_flShootTime = 0.0f;
    }
}

void CZMWeaponRevolver::PrimaryAttackEffects( WeaponSound_t wpnsound )
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer )
        return;

#ifdef CLIENT_DLL
    if ( prediction->IsFirstTimePredicted() )
#endif
    {
        pPlayer->DoMuzzleFlash();

        // We don't want this one... -_-
        //SendWeaponAnim( GetPrimaryAttackActivity() );

        pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

        // ZMRTODO: Right now fires twice. Remove fire sound from model(?)
        WeaponSound( wpnsound );
    }
}

void CZMWeaponRevolver::HandleAnimEventRevolver()
{
    //Shoot();
}

#ifndef CLIENT_DLL
void CZMWeaponRevolver::Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_REVOLVERSHOOT:
		HandleAnimEventRevolver();
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#else
bool CZMWeaponRevolver::OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options )
{
    if ( event == AE_ZM_REVOLVERSHOOT )
    {
        HandleAnimEventRevolver();
        return true;
    }

    return false;
}
#endif

void CZMWeaponRevolver::AddViewKick()
{
    auto* pPlayer = GetPlayerOwner();

    if ( pPlayer && IsInSecondaryAttack() )
    {
        // Disorient the player.
        QAngle angles = pPlayer->GetLocalAngles();

        angles.x += SharedRandomFloat( "revolverx", -2, 2 );
        angles.y += SharedRandomFloat( "revolvery", 2, 4 );

#ifdef CLIENT_DLL
        //pPlayer->SetLocalAngles( angles );
#else
        pPlayer->SnapEyeAngles( angles );
#endif
    }


    BaseClass::AddViewKick();
}
