#include "cbase.h"
#include "zmr_player_ragdoll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( zm_ragdoll, CZMRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CZMRagdoll, DT_ZM_Ragdoll )
    SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1, SPROP_COORD ),
    SendPropEHandle( SENDINFO( m_hPlayer ) ),
    SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
    SendPropInt		( SENDINFO( m_nForceBone ), 8, 0 ),
    SendPropVector	( SENDINFO( m_vecForce ), -1, SPROP_NOSCALE ),
    SendPropVector( SENDINFO( m_vecRagdollVelocity ) ),
END_SEND_TABLE()
