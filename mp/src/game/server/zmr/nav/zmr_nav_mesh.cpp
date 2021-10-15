#include "cbase.h"
#include "props.h"
#include "physobj.h"

#include "npcr/npcr_basenpc.h"
#include "npcr/npcr_manager.h"

#include "zmr_nav_area.h"
#include "zmr_nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// This is basically MASK_SOLID
#define MASK_TRANSIENT          (CONTENTS_SOLID|CONTENTS_GRATE|CONTENTS_WINDOW|CONTENTS_MOVEABLE|CONTENTS_MONSTER)


#define ZMR_NAV_TRANSIENT_HEIGHT    17.0f


ConVar zm_sv_debug_nav_transient( "zm_sv_debug_nav_transient", "0" );
ConVar zm_sv_debug_nav_transient_nofloor( "zm_sv_debug_nav_transient_nofloor", "0" );
ConVar zm_sv_debug_nav_block( "zm_sv_debug_nav_block", "0" );


CZMNavTransientFilter::CZMNavTransientFilter() : CTraceFilterSimple( nullptr, COLLISION_GROUP_NONE, nullptr )
{
}

bool CZMNavTransientFilter::ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask )
{
    if ( !pHandleEntity ) return false;
    auto* pEnt = EntityFromEntityHandle( pHandleEntity );
    if ( !pEnt ) return false;

    // World will never move, so assume the nav mesh creator knows this.
    if ( pEnt->IsWorld() ) return false;

    // Player / npc
    if ( pEnt->IsCombatCharacter() ) return false;

    if ( !pEnt->IsSolid() ) return false;

    //auto* pParent = pEnt->GetRootMoveParent();
    //if ( pParent )
    //    pEnt = pParent;
        
    // ZMRTODO: Are we big enough to block mesh?
    if ( pEnt->IsBSPModel() )
    {
        auto* pPhysBox = dynamic_cast<CPhysBox*>( pEnt );
        if ( pPhysBox ) return false;
    }
    else
    {
        auto* pPhysProp = dynamic_cast<CPhysicsProp*>( pEnt );
        if ( pPhysProp ) return false;
    }


    return true;
}


CZMRNavMesh::CZMRNavMesh()
{
    ListenForGameEvent( "round_restart_pre" );
    ListenForGameEvent( "round_restart_post" );
    ListenForGameEvent( "nav_generate" );
    ListenForGameEvent( "nav_blocked" );
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

    CZMNavTransientFilter filter;
    Vector mins, maxs, center;

    const bool bDebugging = zm_sv_debug_nav_transient.GetFloat() > 0.0f;


    FOR_EACH_VEC( areas, i )
    {
        auto* area = static_cast<CZMRNavArea*>( areas[i] );

        // No floor areas are updated separately.
        if ( area->GetAttributes() & NAV_MESH_ZMR_NOFLOOR )
            continue;


        area->GetWorldBounds( mins, maxs );


        // Trace right above us
        float offset = GetTransientCheckStartHeight();
        mins.z = maxs.z + offset;
        maxs.z = mins.z + 18.0f;


        center = mins + ((maxs - mins) * 0.5f);

        mins = mins - center;
        maxs = maxs - center;
        

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
        else if ( area->IsBlocked( TEAM_ANY ) )
        {
            area->UnblockArea( TEAM_ANY );
        }
    }
}

void CZMRNavMesh::UpdateFloorCheckAreas()
{
    auto& areas = GetTransientAreas();

    CZMNavTransientFilter filter;
    Vector mins, maxs, center;

    const bool bDebugging = zm_sv_debug_nav_transient_nofloor.GetFloat() > 0.0f;


    FOR_EACH_VEC( areas, i )
    {
        auto area = static_cast<CZMRNavArea*>( areas[i] );

        if ( !(area->GetAttributes() & NAV_MESH_ZMR_NOFLOOR) )
            continue;


        area->GetWorldBounds( mins, maxs );

        // Trace right below us
        float offset = GetTransientCheckStartHeight();
        maxs.z = mins.z;
        mins.z = mins.z - offset;


        center = mins + ((maxs - mins) * 0.5f);

        mins = mins - center;
        maxs = maxs - center;


        trace_t tr;

        UTIL_TraceHull( center, center, mins, maxs, MASK_TRANSIENT, &filter, &tr );


        bool bBlock = !tr.m_pEnt;


        if ( bDebugging )
        {
            NDebugOverlay::SweptBox( center, center, mins, maxs, vec3_angle, bBlock ? 255 : 0, (!bBlock) ? 255 : 0, 0, 255, zm_sv_debug_nav_transient_nofloor.GetFloat() );
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


    m_UpdateTransientTimer.Invalidate();
    m_UpdateCheckFloorTimer.Invalidate();
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

    if ( FStrEq( pEvent->GetName(), "nav_blocked" ) )
    {
        if ( zm_sv_debug_nav_block.GetBool() )
        {
            Msg( "Nav area %i block state: %i\n", pEvent->GetInt( "area" ), pEvent->GetInt( "blocked" ) );
        }

        return;
    }


    BaseClass::FireGameEvent( pEvent );
}

CON_COMMAND_F( zm_nav_checknofloor, "", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

    TheNavMesh->CommandNavToggleAttribute( (NavAttributeType)NAV_MESH_ZMR_NOFLOOR );
}
