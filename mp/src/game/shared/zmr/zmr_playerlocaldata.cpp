#include "cbase.h"

#include "zmr_playerlocaldata.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
BEGIN_SEND_TABLE_NOBASE( CZMPlayerLocalData, DT_ZM_PlyLocal )
    SendPropInt( SENDINFO( m_fWeaponSlotFlags ), -1, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_nResources ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
    SendPropFloat( SENDINFO( m_flFlashlightBattery ), 10, SPROP_UNSIGNED | SPROP_ROUNDUP, 0.0f, 100.0f ),
    SendPropFloat( SENDINFO( m_flAccuracyRatio ), 8, SPROP_CHANGES_OFTEN, 0.0f, 1.0f ),

    SendPropEHandle( SENDINFO( m_hLadder ) ),
END_SEND_TABLE()
#else
BEGIN_RECV_TABLE_NOBASE( CZMPlayerLocalData, DT_ZM_PlyLocal )
    RecvPropInt( RECVINFO( m_fWeaponSlotFlags ), SPROP_UNSIGNED ),
    RecvPropInt( RECVINFO( m_nResources ), SPROP_UNSIGNED ),
    RecvPropFloat( RECVINFO( m_flFlashlightBattery ), SPROP_UNSIGNED | SPROP_ROUNDUP ),
    RecvPropFloat( RECVINFO( m_flAccuracyRatio ) ),

    RecvPropEHandle( RECVINFO( m_hLadder ) ),
END_RECV_TABLE()
#endif

CZMPlayerLocalData::CZMPlayerLocalData()
{
    m_fWeaponSlotFlags = 0;
    m_flFlashlightBattery = 100.0f;
    m_nResources = 0;
    m_flAccuracyRatio = 0.0f;

    m_hLadder.Set( nullptr );
}
