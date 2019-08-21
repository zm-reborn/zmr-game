#include "cbase.h"
#include "in_buttons.h"
#include "eventlist.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr_basepump.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED( ZMBasePumpWeapon, DT_ZM_BasePumpWeapon )

BEGIN_NETWORK_TABLE( CZMBasePumpWeapon, DT_ZM_BasePumpWeapon )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_iPumpState ) ),
    RecvPropInt( RECVINFO( m_iReloadState ) ),
    RecvPropBool( RECVINFO( m_bCancelReload ) ),
#else
    SendPropInt( SENDINFO( m_iPumpState ), Q_log2( PUMPSTATE_MAX ) + 1, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_iReloadState ), Q_log2( RELOADSTATE_MAX ) + 1, SPROP_UNSIGNED ),
    SendPropBool( SENDINFO( m_bCancelReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMBasePumpWeapon )
    DEFINE_PRED_FIELD( m_iPumpState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_iReloadState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_bCancelReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_basepump, CZMBasePumpWeapon );


CZMBasePumpWeapon::CZMBasePumpWeapon()
{
    m_bReloadsSingly = true;

    m_iPumpState = PUMPSTATE_NONE;
    m_iReloadState = RELOADSTATE_NONE;
    m_bCancelReload = false;
}

bool CZMBasePumpWeapon::Holster( CBaseCombatWeapon* pSwitchTo )
{
    bool res = BaseClass::Holster( pSwitchTo );

    if ( res )
    {
        m_iReloadState = RELOADSTATE_NONE;
        m_bCancelReload = false;
    }

    return res;
}

void CZMBasePumpWeapon::Shoot( int iAmmoType, int nBullets, int nAmmo, float flMaxRange, bool bSecondary )
{
    m_iPumpState = PUMPSTATE_PUMP_EJECT;

    BaseClass::Shoot( iAmmoType, nBullets, nAmmo, flMaxRange, bSecondary );
}

void CZMBasePumpWeapon::ItemPostFrame()
{
    if ( IsInReload() )
    {
        // Check if player wants to cancel the reload.
        CZMPlayer* pOwner = GetPlayerOwner();
        if ( pOwner && pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) )
        {
            m_bCancelReload = true;
        }
    }


    if ( m_iReloadState == RELOADSTATE_START && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        Reload();
    }


    if ( !IsInReload() && NeedsPump() && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        Pump();
    }


    BaseClass::ItemPostFrame();
}

void CZMBasePumpWeapon::Pump()
{
    CZMPlayer* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;

    
    
    WeaponSound( SPECIAL1 );

    // Finish reload animation
    SendWeaponAnim( m_iPumpState == PUMPSTATE_PUMP_EJECT ? GetPumpAct() : GetEmptyPumpAct() );
    

    float flSeqTime = SequenceDuration();

    float flReadyTime = GetFirstInstanceOfAnimEventTime( GetSequence(), (int)AE_WPN_PRIMARYATTACK, true );
    if ( flReadyTime == -1.0f )
        flReadyTime = flSeqTime;

    
    pOwner->m_flNextAttack = gpGlobals->curtime + flReadyTime;
    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flReadyTime;

    m_iPumpState = PUMPSTATE_NONE;
}

void CZMBasePumpWeapon::CheckReload()
{
    if ( !CanAct( WEPACTION_RELOAD ) )
    {
        if ( m_bInReload2 )
            StopReload();

        return;
    }

    BaseClass::CheckReload();
}

void CZMBasePumpWeapon::StartReload()
{
    if ( m_iReloadState != RELOADSTATE_NONE ) return;

    if ( NeedsPump() ) return;

    if ( m_bInReload2 ) return;

    if ( gpGlobals->curtime < m_flNextPrimaryAttack ) return;

    if ( Clip1() >= GetMaxClip1() ) return;

    if ( !CanAct( WEPACTION_RELOAD ) ) return;

    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    if ( pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) < 1 ) return;


    SendWeaponAnim( GetReloadStartAct() );


    float nextattack = gpGlobals->curtime + SequenceDuration();
    pPlayer->SetNextAttack( nextattack );
    m_flNextPrimaryAttack = nextattack;


    m_iReloadState = RELOADSTATE_START;
}

void CZMBasePumpWeapon::StopReload()
{
    // We no longer want to cancel the reload.
    m_bCancelReload = false;


    SendWeaponAnim( GetReloadEndAct() );

    float nextattack = gpGlobals->curtime + SequenceDuration();

    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( pPlayer )
    {
        pPlayer->SetNextAttack( nextattack );
    }

    m_flNextPrimaryAttack = nextattack;

    
    m_iReloadState = RELOADSTATE_NONE;


    BaseClass::StopReload();
}

void CZMBasePumpWeapon::FinishReload()
{
    BaseClass::FinishReload();


    StopReload();
}

bool CZMBasePumpWeapon::Reload()
{
    if ( m_iReloadState == RELOADSTATE_NONE )
    {
        if ( !NeedsPump() )
        {
            StartReload();
        }

        return false;
    }

    bool res = BaseClass::Reload();

    if ( res )
    {
        m_iReloadState = RELOADSTATE_RELOADING;

        // Keep this commented for now.
        //if ( Clip1() <= 0 )
        //    m_iPumpState = PUMPSTATE_PUMP_EMPTY;
    }
    else if ( m_iReloadState != RELOADSTATE_NONE )
    {
        StopReload();
    }

    return res;
}

bool CZMBasePumpWeapon::IsInReload() const
{
    if ( m_iReloadState != RELOADSTATE_NONE )
        return true;

    return BaseClass::IsInReload();
}

void CZMBasePumpWeapon::CancelReload()
{
    StopReload();
}

bool CZMBasePumpWeapon::ShouldCancelReload() const
{
    CZMPlayer* pOwner = GetPlayerOwner();
    if  ( !pOwner )
        return false;

    if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
        return true;

    if ( m_bCancelReload )
    {
        // Starting or we just ended reloading.
        return  m_iReloadState == RELOADSTATE_START
            ||  (m_flNextPrimaryAttack <= gpGlobals->curtime && m_iClip1 < GetMaxClip1());
    }

    return false;
}
