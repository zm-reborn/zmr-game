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
    DECLARE_ACTTABLE();

    CZMWeaponRifle();
    ~CZMWeaponRifle();


    virtual Vector GetBulletSpread() const OVERRIDE
    {
        Vector cone = BaseClass::GetBulletSpread();

        CZMPlayer* pOwner = GetPlayerOwner();
        if ( pOwner )
        {
            float ratio = 1.0f - pOwner->GetAccuracyRatio();
            ratio *= ratio;

            cone.x = ratio * cone.x;
            cone.y = ratio * cone.y;
            cone.z = ratio * cone.z;
        }


        return cone;
    }


    virtual bool Holster( CBaseCombatWeapon* pSwitchTo = nullptr ) OVERRIDE;
    virtual void Drop( const Vector& vecVelocity ) OVERRIDE;
    virtual void ItemBusyFrame() OVERRIDE;
    virtual void ItemPostFrame() OVERRIDE;


    virtual Activity GetReloadStartAct() OVERRIDE { return ACT_VM_RELOAD_START; }
    virtual Activity GetReloadEndAct() OVERRIDE { return ACT_VM_RELOAD_FINISH; }
    virtual Activity GetPumpAct() OVERRIDE { return ACT_SHOTGUN_PUMP; }

    inline bool IsZoomed() const { return m_bInZoom; }
    void CheckToggleZoom();
    void CheckUnZoom();
    void ToggleZoom();
    void Zoom( CZMPlayer* pOwner ); // ZMRTODO: Do something more reasonable...
    void UnZoom( CZMPlayer* pOwner );

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

acttable_t	CZMWeaponRifle::m_acttable[] = 
{
    /*
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_AR2,                 false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,          false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_AR2,                  false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_AR2,          false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_AR2,       false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_AR2,       false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_AR2,                 false },
};
IMPLEMENT_ACTTABLE( CZMWeaponRifle );

CZMWeaponRifle::CZMWeaponRifle()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_RIFLE );

    m_bInZoom = false;
}

CZMWeaponRifle::~CZMWeaponRifle()
{
    CheckUnZoom();
}

void CZMWeaponRifle::ItemBusyFrame()
{
    CheckToggleZoom();

    BaseClass::ItemBusyFrame();
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
    // We always need to unzoom here even when we don't have a player holding us.
    bool bWasZoomed = IsZoomed();
    if ( bWasZoomed )
        m_bInZoom = false;


    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( pPlayer && bWasZoomed )
    {
        UnZoom( pPlayer );
    }
}

bool CZMWeaponRifle::Holster( CBaseCombatWeapon* pSwitchTo )
{
    CheckUnZoom();

    return BaseClass::Holster( pSwitchTo );
}

void CZMWeaponRifle::Drop( const Vector& vecVelocity )
{
    CheckUnZoom();

    BaseClass::Drop( vecVelocity );
}
