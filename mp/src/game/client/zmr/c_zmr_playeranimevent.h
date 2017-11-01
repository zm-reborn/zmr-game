#pragma once

#include "c_basetempentity.h"

#include "c_zmr_player.h"


class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
    DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
    DECLARE_CLIENTCLASS();

    virtual void PostDataUpdate( DataUpdateType_t updateType )
    {
        // Create the effect.
        C_ZMPlayer* pPlayer = dynamic_cast<C_ZMPlayer*>( m_hPlayer.Get() );
        if ( pPlayer && !pPlayer->IsDormant() )
        {
            pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
        }	
    }

public:
    CNetworkHandle( C_BasePlayer, m_hPlayer );
    CNetworkVar( int, m_iEvent );
    CNetworkVar( int, m_nData );
};
