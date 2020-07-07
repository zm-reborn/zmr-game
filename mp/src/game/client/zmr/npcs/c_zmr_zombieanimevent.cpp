#include "cbase.h"

#include "c_zmr_zombieanimevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_EVENT( C_ZMTEZombieAnimEvent, DT_ZM_TEZombieAnimEvent, CZMTEZombieAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_ZMTEZombieAnimEvent, DT_ZM_TEZombieAnimEvent )
    RecvPropEHandle( RECVINFO( m_hZombie ) ),
    RecvPropInt( RECVINFO( m_iEvent ) ),
    RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()

void C_ZMTEZombieAnimEvent::PostDataUpdate( DataUpdateType_t updateType )
{
    // Create the effect.
    C_ZMBaseZombie* pZombie = static_cast<C_ZMBaseZombie*>( m_hZombie.Get() );
    if ( pZombie && !pZombie->IsDormant() )
    {
        pZombie->DoAnimationEvent( m_iEvent.Get(), m_nData.Get() );
    }	
}
