#include "cbase.h"


#include "npcr/npcr_basenpc.h"
#include "npcr/npcr_manager.h"

#include "zmr_nav_area.h"
#include "zmr_nav_mesh.h"


#define MASK_TRANSIENT          (CONTENTS_SOLID|CONTENTS_MOVEABLE)


#define ZMR_NAV_TRANSIENT_HEIGHT    17.0f


ConVar zm_sv_debug_nav_transient( "zm_sv_debug_nav_transient", "0" );



CZMRNavMesh::CZMRNavMesh()
{
    ListenForGameEvent( "round_restart_pre" );
    ListenForGameEvent( "round_restart_post" );
    ListenForGameEvent( "nav_generate" );
}

CZMRNavMesh::~CZMRNavMesh()
{
}

void CZMRNavMesh::Update()
{
    BaseClass::Update();

    if ( m_UpdateTransientTimer.IsElapsed() || !m_UpdateTransientTimer.HasStarted() )
    {
        UpdateTransientAreas();

        m_UpdateTransientTimer.Start( 1.0f );
    }

    if ( m_UpdateCheckFloorTimer.IsElapsed() || !m_UpdateCheckFloorTimer.HasStarted() )
    {
        UpdateFloorCheckAreas();

        m_UpdateCheckFloorTimer.Start( 1.333f );
    }
}

void CZMRNavMesh::UpdateTransientAreas()
{
    auto& areas = GetTransientAreas();
    auto* pWorld = GetContainingEntity( INDEXENT( 0 ) );

    Vector mins, maxs, center;

    const bool bDebugging = zm_sv_debug_nav_transient.GetBool();


    FOR_EACH_VEC( areas, i )
    {
        auto* area = areas[i];

        if ( area->GetAttributes() & NAV_MESH_ZMR_NOFLOOR )
            continue;
        if ( area->IsBlocked( TEAM_ANY ) )
            continue;


        GetAreaBounds( area, mins, maxs );


        // Trace right above us
        float offset = GetTransientCheckStartHeight();
        mins.z = maxs.z + offset;
        maxs.z = mins.z + 18.0f;


        center = mins + ((maxs - mins) * 0.5f);

        mins = mins - center;
        maxs = maxs - center;
        

        CTraceFilterSimple filter( pWorld, COLLISION_GROUP_NONE );
        trace_t tr;
        
        UTIL_TraceHull( center, center, mins, maxs, MASK_TRANSIENT, &filter, &tr );

        bool bBlock = tr.fraction != 1.0f || tr.startsolid || tr.m_pEnt;



        if ( bDebugging )
        {
            NDebugOverlay::SweptBox( center, center, mins, maxs, vec3_angle, bBlock ? 255 : 0, (!bBlock) ? 255 : 0, 0, 255, zm_sv_debug_nav_transient.GetFloat() );
        }


        if ( bBlock )
        {
            area->MarkAsBlocked( TEAM_ANY, nullptr );
        }
    }
}

void CZMRNavMesh::UpdateFloorCheckAreas()
{
    auto& areas = GetTransientAreas();
    auto* pWorld = GetContainingEntity( INDEXENT( 0 ) );

    Vector mins, maxs, center;

    const bool bDebugging = zm_sv_debug_nav_transient.GetBool();


    FOR_EACH_VEC( areas, i )
    {
        auto area = static_cast<CZMRNavArea*>( areas[i] );

        if ( !(area->GetAttributes() & NAV_MESH_ZMR_NOFLOOR) )
            continue;


        GetAreaBounds( area, mins, maxs );

        // Trace right below us
        float offset = GetTransientCheckStartHeight();
        maxs.z = mins.z;
        mins.z = mins.z - offset;


        center = mins + ((maxs - mins) * 0.5f);

        mins = mins - center;
        maxs = maxs - center;


        CTraceFilterSimple filter( pWorld, COLLISION_GROUP_NONE );
        trace_t tr;

        UTIL_TraceHull( center, center, mins, maxs, MASK_TRANSIENT, &filter, &tr );


        bool bBlock = !tr.m_pEnt;


        if ( bDebugging )
        {
            NDebugOverlay::SweptBox( center, center, mins, maxs, vec3_angle, bBlock ? 255 : 0, (!bBlock) ? 255 : 0, 0, 255, zm_sv_debug_nav_transient.GetFloat() );
        }


        area->SetNoFloor( bBlock );
    }
}

CNavArea* CZMRNavMesh::CreateArea() const
{
    return new CZMRNavArea;
}

void CZMRNavMesh::OnServerActivate()
{
    BaseClass::OnServerActivate();

    // This is post nav file load.
    if ( !IsLoaded() )
    {
        Warning( "!!\n!! Map does not have a NAV mesh loaded!\n!!\n" );
    }
}

class NavRoundRestart
{
public:
    bool operator()( CNavArea *area )
    {
        area->OnRoundRestart();
        return true;
    }

    bool operator()( CNavLadder *ladder )
    {
        ladder->OnRoundRestart();
        return true;
    }
};

void CZMRNavMesh::FireGameEvent( IGameEvent* pEvent )
{
    if ( FStrEq( pEvent->GetName(), "round_restart_pre" ) )
    {
        OnRoundRestartPreEntity();

        FOR_EACH_VEC( TheNavAreas, it )
        {
            CNavArea* area = TheNavAreas[it];
            area->OnRoundRestartPreEntity();
        }

        return;
    }

    if ( FStrEq( pEvent->GetName(), "round_restart_post" ) )
    {
        OnRoundRestart();
        
        NavRoundRestart restart;
        ForAllAreas( restart );
        ForAllLadders( restart );

        return;
    }

    if ( FStrEq( pEvent->GetName(), "nav_generate" ) )
    {
        engine->ServerCommand( "npcr_remove_all\n" );
        return;
    }


    BaseClass::FireGameEvent( pEvent );
}

void CZMRNavMesh::GetAreaBounds( CNavArea* pArea, Vector& mins, Vector& maxs )
{
    Vector temp;

    mins = Vector( FLT_MAX, FLT_MAX, FLT_MAX );
    maxs = Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX );

    // Find heights for proper bounds.
    for ( int j = 0; j < NUM_CORNERS; j++ )
    {
        temp = pArea->GetCorner( (NavCornerType)j );
        if ( temp.x < mins.x )
            mins.x = temp.x;
        if ( temp.y < mins.y )
            mins.y = temp.y;
        if ( temp.z < mins.z )
            mins.z = temp.z;

        if ( temp.x > maxs.x )
            maxs.x = temp.x;
        if ( temp.y > maxs.y )
            maxs.y = temp.y;
        if ( temp.z > maxs.z )
            maxs.z = temp.z;
    }
}

CON_COMMAND_F( zm_nav_checknofloor, "", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

    TheNavMesh->CommandNavToggleAttribute( (NavAttributeType)NAV_MESH_ZMR_NOFLOOR );
}
