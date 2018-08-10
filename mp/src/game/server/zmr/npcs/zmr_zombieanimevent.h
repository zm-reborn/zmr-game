#pragma once

#include "zmr/npcs/zmr_zombiebase_shared.h"


class CZMTEZombieAnimEvent : public CBaseTempEntity
{
public:
    DECLARE_CLASS( CZMTEZombieAnimEvent, CBaseTempEntity );
    DECLARE_SERVERCLASS();

    CZMTEZombieAnimEvent( const char* name ) : CBaseTempEntity( name )
    {
    }

    CNetworkHandle( CZMBaseZombie, m_hZombie );
    CNetworkVar( int, m_iEvent );
    CNetworkVar( int, m_nData );
};

extern void TE_ZombieAnimEvent( CZMBaseZombie* pZombie, ZMZombieAnimEvent_t anim, int nData );
