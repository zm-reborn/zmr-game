#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "ai_activity.h"
#include "in_buttons.h"

#include "zmr_rifle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


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

bool CZMWeaponRifle::CanZoom() const
{
    return m_flNextPrimaryAttack < gpGlobals->curtime;
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
            if ( CanZoom() )
                Zoom( pPlayer );
        }
    }
}

void CZMWeaponRifle::Zoom( CZMPlayer* pPlayer )
{
    pPlayer->SetFOV( this, 35, 0.1f );

    m_bInZoom = true;

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
}

void CZMWeaponRifle::UnZoom( CZMPlayer* pPlayer )
{
    pPlayer->SetFOV( this, 0, 0.1f );

    m_bInZoom = false;

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
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

void CZMWeaponRifle::StartReload()
{
    BaseClass::StartReload();

    // Don't stay zoomed when reloading.
    if ( IsInReload() )
    {
        CheckUnZoom();
    }
}
