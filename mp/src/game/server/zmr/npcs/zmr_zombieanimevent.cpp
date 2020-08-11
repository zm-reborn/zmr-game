#include "cbase.h"

#include "Multiplayer/multiplayer_animstate.h"
#include "npcs/zmr_zombieanimstate.h"

#include "zmr_zombieanimevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static CZMTEZombieAnimEvent g_ZMTEZombieAnimEvent( "ZombieAnimEvent" );


IMPLEMENT_SERVERCLASS_ST_NOBASE( CZMTEZombieAnimEvent, DT_ZM_TEZombieAnimEvent )
    SendPropEHandle( SENDINFO( m_hZombie ) ),
    SendPropInt( SENDINFO( m_iEvent ), Q_log2( ZOMBIEANIMEVENT_MAX ) + 1, SPROP_UNSIGNED ),
    // We may send the activity number or a seed for random animations.
    SendPropInt(
        SENDINFO( m_nData ),
        Q_log2( (int)LAST_SHARED_ACTIVITY ) + 1, // We should only be sending max the value of an activity
        SPROP_UNSIGNED ),
END_SEND_TABLE()

void TE_ZombieAnimEvent( CZMBaseZombie* pZombie, ZMZombieAnimEvent_t anim, int nData )
{
    // We need to send this to all players always, because some animations
    // affect the state of the zombie fundamentally.
    // ie. banshee ceiling ambush will have wrong anims if banshee was not in PVS.
    // or just ZM being outside the map and the zombie attacking something.
    CRecipientFilter filter;
    filter.AddAllPlayers();

    g_ZMTEZombieAnimEvent.m_hZombie = pZombie;
    g_ZMTEZombieAnimEvent.m_iEvent = anim;
    g_ZMTEZombieAnimEvent.m_nData = nData;
    g_ZMTEZombieAnimEvent.Create( filter, 0 );
}
