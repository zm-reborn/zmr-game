#pragma once

#include "Multiplayer/multiplayer_animstate.h"


class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
    DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
    DECLARE_SERVERCLASS();

    CTEPlayerAnimEvent( const char* name ) : CBaseTempEntity( name )
    {
    }

    CNetworkHandle( CBasePlayer, m_hPlayer );
    CNetworkVar( int, m_iEvent );
    CNetworkVar( int, m_nData );
};

extern CTEPlayerAnimEvent g_TEPlayerAnimEvent;
