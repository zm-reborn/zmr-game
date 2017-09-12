#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "ai_activity.h"
#include "in_buttons.h"

#include "zmr_basepump.h"


#ifdef CLIENT_DLL
#define CZMWeaponRifle C_ZMWeaponRifle
#endif

class CZMWeaponRifle : public CZMBasePumpWeapon
{
public:
    DECLARE_CLASS( CZMWeaponRifle, CZMBasePumpWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
    DECLARE_ACTTABLE();
#endif

    CZMWeaponRifle();
    ~CZMWeaponRifle();


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_ammo_357"; };
    int GetDropAmmoAmount() OVERRIDE { return SIZE_AMMO_357; };
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


        QAngle viewPunch;

        viewPunch.x = SharedRandomFloat( "riflepax", -10.0f, -6.0f );
        viewPunch.y = SharedRandomFloat( "riflepay", -2.0f, 2.0f );
        viewPunch.z = 0.0f;

        pPlayer->ViewPunch( viewPunch );
    }
    
    virtual float GetFireRate( void ) OVERRIDE { return 0.9f; };


    virtual void PrimaryAttack( void ) OVERRIDE;
    virtual bool Holster( CBaseCombatWeapon* pSwitchTo = nullptr ) OVERRIDE;
    virtual void ItemPostFrame( void ) OVERRIDE;


    virtual Activity GetReloadStartAct() OVERRIDE { return ACT_RIFLE_RELOAD_START; };
    virtual Activity GetReloadEndAct() OVERRIDE { return ACT_RIFLE_RELOAD_FINISH; };
    virtual Activity GetPumpAct() OVERRIDE { return ACT_RIFLE_LEVER; };

    inline bool IsZoomed() { return m_bInZoom; };
    void CheckToggleZoom();
    void CheckUnZoom();
    void ToggleZoom();
    void Zoom( CZMPlayer* ); // ZMRTODO: Do something more reasonable...
    void UnZoom( CZMPlayer* );

protected:
    CNetworkVar( bool, m_bInZoom );
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponRifle, DT_ZM_WeaponRifle )

BEGIN_NETWORK_TABLE( CZMWeaponRifle, DT_ZM_WeaponRifle )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bInZoom ) ),
#else
    SendPropBool( SENDINFO( m_bInZoom ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponRifle )
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


#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
#endif

    m_bInZoom = false;
}

CZMWeaponRifle::~CZMWeaponRifle()
{
    CheckUnZoom();
}

void CZMWeaponRifle::PrimaryAttack( void )
{
    m_bNeedPump = true;


    BaseClass::PrimaryAttack();
}

void CZMWeaponRifle::ItemPostFrame( void )
{
    CheckToggleZoom();

    BaseClass::ItemPostFrame();
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

bool CZMWeaponRifle::Holster( CBaseCombatWeapon* pSwitchTo )
{
    CheckUnZoom();

    return BaseClass::Holster( pSwitchTo );
}
