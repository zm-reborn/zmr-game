#pragma once

#include "cbase.h"


class CZMRagdoll : public CBaseAnimatingOverlay
{
public:
    DECLARE_CLASS( CZMRagdoll, CBaseAnimatingOverlay );
    DECLARE_SERVERCLASS();

    // Transmit ragdolls to everyone.
    virtual int UpdateTransmitState()
    {
        return SetTransmitState( FL_EDICT_ALWAYS );
    }

public:
    // In case the client has the player entity, we transmit the player index.
    // In case the client doesn't have it, we transmit the player's model index, origin, and angles
    // so they can create a ragdoll in the right place.
    CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
    CNetworkVector( m_vecRagdollVelocity );
    CNetworkVector( m_vecRagdollOrigin );
};
