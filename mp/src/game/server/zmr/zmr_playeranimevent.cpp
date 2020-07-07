#include "cbase.h"

#include "zmr_playeranimevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
    SendPropEHandle( SENDINFO( m_hPlayer ) ),
    SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

void TE_PlayerAnimEvent( CBasePlayer* pPlayer, PlayerAnimEvent_t playerAnim, int nData )
{
    CPVSFilter filter( (const Vector&)pPlayer->WorldSpaceCenter() );

    //Tony; use prediction rules.
    filter.UsePredictionRules();
    
    g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
    g_TEPlayerAnimEvent.m_iEvent = playerAnim;
    g_TEPlayerAnimEvent.m_nData = nData;
    g_TEPlayerAnimEvent.Create( filter, 0 );
}

CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );
