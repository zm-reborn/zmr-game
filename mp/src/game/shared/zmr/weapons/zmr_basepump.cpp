#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr_basepump.h"


IMPLEMENT_NETWORKCLASS_ALIASED( ZMBasePumpWeapon, DT_ZM_BasePumpWeapon )

BEGIN_NETWORK_TABLE( CZMBasePumpWeapon, DT_ZM_BasePumpWeapon )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bNeedPump ) ),
    RecvPropInt( RECVINFO( m_iReloadState ) ),
#else
    SendPropBool( SENDINFO( m_bNeedPump ) ),
    SendPropInt( SENDINFO( m_iReloadState ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMBasePumpWeapon )
    DEFINE_PRED_FIELD( m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_iReloadState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_basepump, CZMBasePumpWeapon );


CZMBasePumpWeapon::CZMBasePumpWeapon()
{
    m_bReloadsSingly = true;

    m_bNeedPump = false;
    m_iReloadState = RELOADSTATE_NONE;
}

bool CZMBasePumpWeapon::Holster( CBaseCombatWeapon* pSwitchTo )
{
    bool res = BaseClass::Holster( pSwitchTo );

    if ( res )
    {
        m_iReloadState = RELOADSTATE_NONE;
    }

    return res;
}

void CZMBasePumpWeapon::ItemPostFrame( void )
{
    if ( m_bNeedPump && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        Pump();
    }


    if ( m_iReloadState == RELOADSTATE_START && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        Reload();
    }


    BaseClass::ItemPostFrame();
}

void CZMBasePumpWeapon::Pump()
{
    CZMPlayer* pOwner = GetPlayerOwner();

    if ( !pOwner ) return;

    
    m_bNeedPump = false;
    
    WeaponSound( SPECIAL1 );

    // Finish reload animation
    SendWeaponAnim( GetPumpAct() );
    
    float delay = SequenceDuration();
    pOwner->m_flNextAttack = gpGlobals->curtime + delay;
    m_flNextPrimaryAttack = gpGlobals->curtime + delay;
}

void CZMBasePumpWeapon::StartReload( void )
{
    if ( m_iReloadState != RELOADSTATE_NONE ) return;

    if ( m_bNeedPump ) return;

    if ( m_bInReload ) return;

    if ( gpGlobals->curtime < m_flNextPrimaryAttack ) return;

    if ( Clip1() >= GetMaxClip1() ) return;

    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;

    if ( pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) < 1 ) return;


    SendWeaponAnim( GetReloadStartAct() );


    float nextattack = gpGlobals->curtime + SequenceDuration();
    pPlayer->SetNextAttack( nextattack );
    m_flNextPrimaryAttack = nextattack;


    m_iReloadState = RELOADSTATE_START;
}

void CZMBasePumpWeapon::FinishReload( void )
{
    BaseClass::FinishReload();


    SendWeaponAnim( GetReloadEndAct() );

    float nextattack = gpGlobals->curtime + SequenceDuration();

    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( pPlayer )
    {
        pPlayer->SetNextAttack( nextattack );
    }

    m_flNextPrimaryAttack = nextattack;

    
    m_iReloadState = RELOADSTATE_NONE;
}

bool CZMBasePumpWeapon::Reload( void )
{
    if ( m_iReloadState != RELOADSTATE_RELOADING )
    {
        if ( m_bNeedPump ) return false;

        if ( m_iReloadState == RELOADSTATE_NONE )
        {
            StartReload();
            return false;
        }
    }


    bool res = BaseClass::Reload();

    if ( res )
    {
        m_iReloadState = RELOADSTATE_RELOADING;
    }

    return res;
}

bool CZMBasePumpWeapon::IsInReload()
{
    if ( m_iReloadState != RELOADSTATE_NONE )
        return true;

    return BaseClass::IsInReload();
}
