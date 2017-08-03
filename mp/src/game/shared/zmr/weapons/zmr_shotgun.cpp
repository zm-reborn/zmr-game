#include "cbase.h"

#include "zmr/zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr/zmr_player_shared.h"


#ifdef CLIENT_DLL
#define CZMWeaponShotgun C_ZMWeaponShotgun
#endif

class CZMWeaponShotgun : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponShotgun, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

    CZMWeaponShotgun();


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_box_buckshot"; };
    int GetDropAmmoAmount() OVERRIDE { return 8; };
#endif

    virtual const Vector& GetBulletSpread( void ) OVERRIDE
    {
        static Vector cone = VECTOR_CONE_10DEGREES;
        return cone;
    }
    
    virtual void AddViewKick( void ) OVERRIDE
    {
        CZMPlayer* pPlayer = ToZMPlayer( GetOwner() );

        if ( !pPlayer ) return;


	    pPlayer->ViewPunch( QAngle( random->RandomFloat( -2, -1 ), random->RandomFloat( -2, 2 ), 0 ) );
    }
    
    virtual float GetFireRate( void ) OVERRIDE { return 0.7f; };



    virtual bool Reload( void ) OVERRIDE;
    virtual void PrimaryAttack( void ) OVERRIDE;
    virtual void ItemPostFrame( void ) OVERRIDE;

    virtual void Pump();

protected:
    CNetworkVar( bool, m_bNeedPump ); // When emptied completely
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponShotgun, DT_ZM_WeaponShotgun )

BEGIN_NETWORK_TABLE( CZMWeaponShotgun, DT_ZM_WeaponShotgun )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bNeedPump ) ),
#else
    SendPropBool( SENDINFO( m_bNeedPump ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponShotgun )
    DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_shotgun, CZMWeaponShotgun );
PRECACHE_WEAPON_REGISTER( weapon_zm_shotgun );

#ifndef CLIENT_DLL
acttable_t CZMWeaponShotgun::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
};
IMPLEMENT_ACTTABLE( CZMWeaponShotgun );
#endif

CZMWeaponShotgun::CZMWeaponShotgun()
{
    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 500.0f;

    m_bFiresUnderwater = false;


    m_bReloadsSingly = true;

#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
#endif


    m_bNeedPump = false;
}

void CZMWeaponShotgun::PrimaryAttack( void )
{
    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    m_bNeedPump = true;


	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );


	FireBulletsInfo_t info(
        7,
        pPlayer->Weapon_ShootPosition(),
        ((CBasePlayer*)pPlayer)->GetAutoaimVector( AUTOAIM_10DEGREES ),
        GetBulletSpread(),
        MAX_TRACE_LENGTH,
        m_iPrimaryAmmoType );

	info.m_pAttacker = pPlayer;
    
	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );
	
	QAngle punch;
	punch.Init( SharedRandomFloat( "shotgunpax", -2, -1 ), SharedRandomFloat( "shotgunpay", -2, 2 ), 0 );
	pPlayer->ViewPunch( punch );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}
}

void CZMWeaponShotgun::ItemPostFrame( void )
{
    if ( m_bNeedPump && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
    {
        Pump();
    }

    BaseClass::ItemPostFrame();
}

void CZMWeaponShotgun::Pump()
{
    CZMPlayer* pOwner = GetPlayerOwner();

    if ( !pOwner ) return;
	
    m_bNeedPump = false;

    /*if ( m_bDelayedReload )
    {
	    m_bDelayedReload = false;
	    StartReload();
    }*/
	
    WeaponSound( SPECIAL1 );

    // Finish reload animation
    SendWeaponAnim( ACT_SHOTGUN_PUMP );

    pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
    m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

bool CZMWeaponShotgun::Reload( void )
{
    if ( m_bNeedPump ) return false;

    if ( m_flNextPrimaryAttack > gpGlobals->curtime ) return false;

    return BaseClass::Reload();
}