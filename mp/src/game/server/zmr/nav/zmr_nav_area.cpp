#include "cbase.h"

#include "zmr_nav_area.h"


CZMRNavArea::CZMRNavArea()
{
    m_bHasNoFloor = false;
}

CZMRNavArea::~CZMRNavArea()
{
}

void CZMRNavArea::ComputeHidingSpots()
{
}

void CZMRNavArea::ComputeSniperSpots()
{
}

void CZMRNavArea::ComputeSpotEncounters()
{
}

void CZMRNavArea::UpdateBlocked( bool force, int teamID )
{
    //if ( !force && m_BlockedTimer.IsElapsed() )
    //{
    //    return;
    //}

    // Transient areas are updated separately.
    if ( HasAttributes( NAV_MESH_TRANSIENT ) )
    {
        return;
    }


    BaseClass::UpdateBlocked( force, teamID );


    /*m_BlockedTimer.Start( 1.0f );

    CZMNavTransientFilter filter;
    Vector mins, maxs, center;


    GetBlockTraceBox( center, mins, maxs );

    trace_t tr;
        
    UTIL_TraceHull( center, center, mins, maxs, MASK_SOLID, &filter, &tr );


    bool bWasBlocked = IsBlocked( TEAM_ANY );
    bool bBlocked = tr.fraction != 1.0f || tr.startsolid || tr.m_pEnt;
    bool bChanged = bBlocked != bWasBlocked;




	if ( bChanged )
	{
        for ( int i = 0; i < 2; i++ )
        {
            m_isBlocked[i] = bBlocked;
        }

        auto* pEvent = gameeventmanager->CreateEvent( "nav_blocked" );
		if ( pEvent )
		{
			pEvent->SetInt( "area", GetID() );
			pEvent->SetInt( "blocked", bBlocked );
			gameeventmanager->FireEvent( pEvent );
		}

		if ( bBlocked )
		{
			TheNavMesh->OnAreaBlocked( this );
		}
		else
		{
			TheNavMesh->OnAreaUnblocked( this );
		}
	}

	if ( TheNavMesh->GetMarkedArea() == this )
	{
		if ( IsBlocked( teamID ) )
		{
			NDebugOverlay::Box( center, mins, maxs, 255, 0, 0, 64, 3.0f );
		}
		else
		{
			NDebugOverlay::Box( center, mins, maxs, 0, 255, 0, 64, 3.0f );
		}
	}*/
}

bool CZMRNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
    return BaseClass::IsBlocked( teamID, ignoreNavBlockers ) || m_bHasNoFloor;
}
