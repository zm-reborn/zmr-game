#include "cbase.h"

#include "ai_activity.h"
#include "in_buttons.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr_base.h"


#include "zmr/zmr_player_shared.h"


#ifdef CLIENT_DLL
#define CZMWeaponRifle C_ZMWeaponRifle
#endif

class CZMWeaponRifle : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponRifle, CZMBaseWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

    CZMWeaponRifle();
    ~CZMWeaponRifle();


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_ammo_357"; };
    int GetDropAmmoAmount() OVERRIDE { return 10; };
#endif


    virtual const Vector& GetBulletSpread( void ) OVERRIDE
    {
        static Vector cone( 0, 0, 0 );
        return cone;
    }
    
    virtual void AddViewKick( void ) OVERRIDE
    {
        CZMPlayer* pPlayer = ToZMPlayer( GetOwner() );

        if ( !pPlayer ) return;


	    pPlayer->ViewPunch( QAngle( random->RandomFloat( -5, -2 ), random->RandomFloat( -3.5, 3.5 ), 0 ) );
    }
    
    virtual float GetFireRate( void ) OVERRIDE { return 1.0f; };



    virtual bool Reload( void ) OVERRIDE;
    virtual void PrimaryAttack( void ) OVERRIDE;
    virtual void ItemPostFrame( void ) OVERRIDE;

    virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );


    virtual void Pump();

    inline bool IsZoomed() { return m_bInZoom; };
    void CheckToggleZoom();
    void CheckUnZoom();
    void ToggleZoom();
    void Zoom( CZMPlayer* ); // ZMRTODO: Do something more reasonable...
    void UnZoom( CZMPlayer* );

protected:
    CNetworkVar( bool, m_bNeedPump ); // When emptied completely
    CNetworkVar( bool, m_bInZoom );
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponRifle, DT_ZM_WeaponRifle )

BEGIN_NETWORK_TABLE( CZMWeaponRifle, DT_ZM_WeaponRifle )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bNeedPump ) ),
    RecvPropBool( RECVINFO( m_bInZoom ) ),
#else
    SendPropBool( SENDINFO( m_bNeedPump ) ),
    SendPropBool( SENDINFO( m_bInZoom ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponRifle )
    DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_rifle, CZMWeaponRifle );
PRECACHE_WEAPON_REGISTER( weapon_zm_rifle );

#ifndef CLIENT_DLL
acttable_t	CZMWeaponRifle::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
};
IMPLEMENT_ACTTABLE( CZMWeaponRifle );
#endif

CZMWeaponRifle::CZMWeaponRifle()
{
    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 2000.0f;

    m_bFiresUnderwater = false;


    m_bReloadsSingly = true;

#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
#endif


    m_bNeedPump = false;
    m_bInZoom = false;
}

CZMWeaponRifle::~CZMWeaponRifle()
{
    CheckUnZoom();
}

void CZMWeaponRifle::PrimaryAttack( void )
{
    m_bNeedPump = true;


    return BaseClass::PrimaryAttack();
}

void CZMWeaponRifle::ItemPostFrame( void )
{
    if ( m_bNeedPump && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
    {
        Pump();
    }


    CheckToggleZoom();

    BaseClass::ItemPostFrame();
}

void CZMWeaponRifle::Pump()
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
    SendWeaponAnim( ACT_RIFLE_LEVER );
    
    //pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

bool CZMWeaponRifle::Reload( void )
{
    if ( m_bNeedPump ) return false;

    if ( m_flNextPrimaryAttack > gpGlobals->curtime ) return false;

    return BaseClass::Reload();
}

void CZMWeaponRifle::CheckToggleZoom()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
    {
        if ( IsZoomed() )
        {
            UnZoom( pPlayer );
        }
        else
        {
            Zoom( pPlayer );
        }
    }
}

void CZMWeaponRifle::Zoom( CZMPlayer* pPlayer )
{
    pPlayer->SetFOV( this, 35, 0.1f );

    m_bInZoom = true;
}

void CZMWeaponRifle::UnZoom( CZMPlayer* pPlayer )
{
    pPlayer->SetFOV( this, 0, 0.2f );

    m_bInZoom = false;
}

void CZMWeaponRifle::CheckUnZoom()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    if ( IsZoomed() )
    {
        pPlayer->SetFOV( this, 0, 0.2f );

        m_bInZoom = false;
    }
}

bool CZMWeaponRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
    CheckUnZoom();

    return BaseClass::Holster( pSwitchingTo );
}