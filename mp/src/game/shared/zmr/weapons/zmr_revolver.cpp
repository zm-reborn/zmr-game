#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"

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
#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

    CZMWeaponRevolver();


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_ammo_revolver"; };
    int GetDropAmmoAmount() OVERRIDE { return SIZE_ZMAMMO_REVOLVER; };
#endif

    virtual const Vector& GetBulletSpread( void ) OVERRIDE
    {
        static Vector cone = Vector( 0.0f, 0.0f, 0.0f );
        return cone;
    }
    
    virtual void AddViewKick( void ) OVERRIDE;

    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;

    void Shoot();
    
    virtual float GetFireRate( void ) OVERRIDE { return 1.0f; };

    void ItemPostFrame() OVERRIDE;

    void HandleAnimEventRevolver( CBaseCombatCharacter* );

#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t*, CBaseCombatCharacter* ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel*, const Vector&, const QAngle&, int event, const char* ) OVERRIDE;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponRevolver, DT_ZM_WeaponRevolver )

BEGIN_NETWORK_TABLE( CZMWeaponRevolver, DT_ZM_WeaponRevolver )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponRevolver )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_revolver, CZMWeaponRevolver );
PRECACHE_WEAPON_REGISTER( weapon_zm_revolver );

#ifndef CLIENT_DLL
acttable_t CZMWeaponRevolver::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};
IMPLEMENT_ACTTABLE( CZMWeaponRevolver );
#endif

CZMWeaponRevolver::CZMWeaponRevolver()
{
    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 1500.0f;

    m_bFiresUnderwater = false;

#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_SIDEARM );
#endif
}

void CZMWeaponRevolver::PrimaryAttack()
{
	if ( UsesClipsForAmmo1() && !Clip1() ) 
	{
		Reload();
		return;
	}


    SendWeaponAnim( ACT_VM_PRIMARYATTACK );

    m_flNextPrimaryAttack = gpGlobals->curtime + 1.1f;
    m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
}

void CZMWeaponRevolver::SecondaryAttack()
{
	if ( UsesClipsForAmmo1() && !Clip1() ) 
	{
		Reload();
		return;
	}

    SendWeaponAnim( ACT_VM_SECONDARYATTACK );

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.4f;
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;
}

void CZMWeaponRevolver::ItemPostFrame()
{
    //CZMPlayer* pPlayer = GetPlayerOwner();
    //if ( !pPlayer ) return;

    BaseClass::ItemPostFrame();
}

void CZMWeaponRevolver::Shoot()
{
	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !Clip1() ) 
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	pPlayer->DoMuzzleFlash();

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition( );
	
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );


	info.m_iShots = 1;

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = MIN( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	FireBullets( info );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	//Add our view kick in
	AddViewKick();
}

void CZMWeaponRevolver::HandleAnimEventRevolver( CBaseCombatCharacter* pOperator )
{
    Shoot();
}

#ifndef CLIENT_DLL
void CZMWeaponRevolver::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_REVOLVERSHOOT:
		HandleAnimEventRevolver( pOperator );
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
        HandleAnimEventRevolver( GetOwner() );
        return true;
    }

    return false;
}
#endif

void CZMWeaponRevolver::AddViewKick()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    Activity act = GetActivity();

#ifndef CLIENT_DLL
    if ( act == ACT_VM_SECONDARYATTACK )
    {
        //Disorient the player
        QAngle angles = pPlayer->GetLocalAngles();

        angles.x += random->RandomInt( -2, 2 );
        angles.y += random->RandomInt( 2, 4 );
        angles.z = 0;
        pPlayer->SnapEyeAngles( angles );
    }
#endif

    if ( act == ACT_VM_PRIMARYATTACK )
        pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -1, 1 ), 0 ) );
    else
        pPlayer->ViewPunch( QAngle( random->RandomFloat( -8, 2 ), random->RandomFloat( -2, 2 ), 0 ) );
}