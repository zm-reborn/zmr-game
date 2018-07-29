#pragma once

#include "c_basetempentity.h"

#include "c_zmr_zombiebase.h"


class C_ZMTEZombieAnimEvent : public C_BaseTempEntity
{
public:
    DECLARE_CLASS( C_ZMTEZombieAnimEvent, C_BaseTempEntity );
    DECLARE_CLIENTCLASS();

    virtual void PostDataUpdate( DataUpdateType_t updateType )
    {
        // Create the effect.
        C_ZMBaseZombie* pZombie = static_cast<C_ZMBaseZombie*>( m_hZombie.Get() );
        if ( pZombie && !pZombie->IsDormant() )
        {
            pZombie->DoAnimationEvent( m_iEvent.Get(), 0 );
        }	
    }

public:
    CNetworkHandle( C_ZMBaseZombie, m_hZombie );
    CNetworkVar( int, m_iEvent );
    //CNetworkVar( int, m_nData );
};
