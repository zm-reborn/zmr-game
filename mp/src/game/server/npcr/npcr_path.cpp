#include "cbase.h"

#include "npcr_path.h"


ConVar npcr_debug_path( "npcr_debug_path", "0" );

ConVar npcr_preferred_navbuildmethod( "npcr_preferred_navbuildmethod", "0", 0, "Preferred build method. 0 = Nav mesh, 1 = AI Graph" );


NPCR::CBaseNavPath::CBaseNavPath()
{
    m_nLinkCount = 0;
    m_bExactGoal = false;
}

NPCR::CBaseNavPath::~CBaseNavPath()
{
}

NPCR::NavBuildMethod_t NPCR::CBaseNavPath::GetBuildMethod()
{
    bool bNavMesh = TheNavMesh->IsLoaded();
    bool bGraph = HasAIGraph();

    if ( !bNavMesh && bGraph )
        return BUILD_AIGRAPH;
    if ( bNavMesh && !bGraph )
        return BUILD_NAVMESH;

    // If we have both, use the preferred.
    return (NavBuildMethod_t)npcr_preferred_navbuildmethod.GetInt();
}

void NPCR::CBaseNavPath::OnPathSuccess()
{

}

void NPCR::CBaseNavPath::OnPathFailed()
{
    Invalidate();
}

bool NPCR::CBaseNavPath::Compute( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const CBasePathCost& cost )
{
    Invalidate();


    bool bPathResult;


    const NavBuildMethod_t buildMethod = GetBuildMethod();

    switch ( buildMethod )
    {
    case BUILD_DIRECT :
        if ( ShouldDraw() )
        {
            DevMsg( "Building direct path...\n" );
        }

        bPathResult = BuildSimplePath( vecStart, vecGoal, true );
        break;
    case BUILD_AIGRAPH :
        if ( ShouldDraw() )
        {
            DevMsg( "Building AI graph path...\n" );
        }

        bPathResult = BuildGraphPath( vecStart, vecGoal, cost );
        break;
    case BUILD_NAVMESH :
    default :
        if ( ShouldDraw() )
        {
            DevMsg( "Building nav mesh path...\n" );
        }

        bPathResult = BuildNavPath( vecStart, vecGoal, pStartArea, pGoalArea, cost );
        break;
    }
    

    return bPathResult;
}

bool NPCR::CBaseNavPath::Compute( const Vector& vecStart, const Vector& vecGoal, const CBasePathCost& cost, float maxDistanceToArea )
{
    Invalidate();

    CNavArea* pStartArea = TheNavMesh->GetNearestNavArea( vecStart, true, maxDistanceToArea, true );
    CNavArea* pGoalArea = TheNavMesh->GetNearestNavArea( vecGoal, true, maxDistanceToArea, true );

    return Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
}

bool NPCR::CBaseNavPath::Compute( CBaseCombatCharacter* pNPC, CBaseCombatCharacter* pTarget, const CBasePathCost& cost )
{
    Invalidate();


    CNavArea* pStartArea = pNPC->GetLastKnownArea();
    CNavArea* pGoalArea = pTarget->GetLastKnownArea();

    const Vector vecStart = pNPC->WorldSpaceCenter();
    const Vector vecGoal = pTarget->WorldSpaceCenter();

    return Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
}

bool NPCR::CBaseNavPath::Compute( CBaseCombatCharacter* pNPC, CBaseEntity* pTarget, const CBasePathCost& cost, float maxDistanceToArea )
{
    Invalidate();


    CNavArea* pStartArea = pNPC->GetLastKnownArea();
    const Vector vecGoal = pTarget->WorldSpaceCenter();
    CNavArea* pGoalArea = TheNavMesh->GetNearestNavArea( vecGoal, true, maxDistanceToArea, true );
    const Vector vecStart = pNPC->WorldSpaceCenter();

    return Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
}

bool NPCR::CBaseNavPath::BuildSimplePath( const Vector& vecStart, const Vector& vecGoal, bool bNoNavArea, float maxDistanceToArea )
{
    bool res = false;

    // Let's try finding the areas at least.
    if ( !bNoNavArea )
    {
        CNavArea* pStart = TheNavMesh->GetNearestNavArea( vecStart, false, maxDistanceToArea, false );
        CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecGoal, false, maxDistanceToArea, false );

        res = BuildSimplePath( vecStart, vecGoal, pStart, pGoal );
    }


    if ( !res )
    {
        // Even THAT failed, just a straight path with no areas.
        m_nLinkCount = 2;

        m_Links[0].area = nullptr;
        m_Links[0].pos = vecStart;
        m_Links[0].navTravel = TRAVEL_ONGROUND;

        m_Links[1].area = nullptr;
        m_Links[1].pos = vecGoal;
        m_Links[1].navTravel = TRAVEL_ONGROUND;


        m_Links[0].fwd = m_Links[1].pos - m_Links[0].pos;
        m_Links[0].fwd_dot = 1.0f;
        m_Links[0].length = m_Links[0].fwd.NormalizeInPlace();

        m_Links[1].fwd = m_Links[1].fwd;
        m_Links[1].fwd_dot = 1.0f;
        m_Links[1].length = 0.0f;

        OnPathCreated();

        return true;
    }

    return false;
}

bool NPCR::CBaseNavPath::BuildSimplePath( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea )
{
    if ( !pStartArea )
        return false;

    if ( !pGoalArea )
        return false;


    m_nLinkCount = 2;

    m_Links[0].area = pStartArea;
    m_Links[0].pos = vecStart;
    m_Links[0].pos.z = pStartArea->GetZ( vecStart );
    m_Links[0].navTravel = TRAVEL_ONGROUND;

    m_Links[1].area = pGoalArea;
    m_Links[1].pos = vecGoal;
    m_Links[1].pos.z = pGoalArea->GetZ( vecGoal );
    m_Links[1].navTravel = TRAVEL_ONGROUND;


    m_Links[0].fwd = m_Links[1].pos - m_Links[0].pos;
    m_Links[0].fwd_dot = 1.0f;
    m_Links[0].length = m_Links[0].fwd.NormalizeInPlace();

    m_Links[1].fwd = m_Links[1].fwd;
    m_Links[1].fwd_dot = 1.0f;
    m_Links[1].length = 0.0f;


    OnPathCreated();

    return true;
}

bool NPCR::CBaseNavPath::ComputeNavPathDetails( int count, const Vector& vecStart, CNavArea* pLastArea, const Vector& vecGoal, const NPCR::CBasePathCost& cost )
{
    if ( count >= MAX_PATH_LINKS-1 )
    {
        count = MAX_PATH_LINKS-1;
    }

    // We need to save room for count + 1
    Assert( count < MAX_PATH_LINKS-1 );
    m_nLinkCount = count;

    // Copy areas over starting from the last to the first.
    CNavArea* area;
    for ( area = pLastArea; count > 0 && area; area = area->GetParent() )
    {
        --count;
        m_Links[count].area = area;
        m_Links[count].navTraverse = area->GetParentHow();
        m_Links[count].navTravel = TRAVEL_ONGROUND;
    }


    NavLink_t* first = &m_Links[0];
    if ( first->area->Contains( vecStart ) )
    {
        first->pos = vecStart;
        first->pos.z = first->area->GetZ( first->pos );
    }
    else
    {
        first->area->GetClosestPointOnArea( vecStart, &first->pos );

        Vector dirToStart = first->pos - vecStart;
        float distToStart = dirToStart.NormalizeInPlace();

        Vector dirToEnd = vecGoal - vecStart;
        float distToEnd = dirToEnd.NormalizeInPlace();

        // We're closer to end than to the start
        // Or we're going the other way than we should!
        if ( distToStart > distToEnd || dirToStart.Dot( dirToEnd ) < -0.1f )
        {
            first->pos = vecStart;
        }
    }

    m_Links[0].fwd_dot = 1.0f;


    int i = 1;
    for ( ; i < m_nLinkCount; i++ )
    {
        NavLink_t* from = &m_Links[i-1];
        NavLink_t* to = &m_Links[i];

        // Only handle ground traveling for now.
        Assert( to->navTraverse <= GO_WEST );

        from->area->ComputePortal( to->area, (NavDirType)to->navTraverse, &to->portalCenter, &to->portalHalfWidth );

        from->area->ComputeClosestPointInPortal( to->area, (NavDirType)to->navTraverse, from->pos, &to->pos );
        to->pos.z = from->area->GetZ( to->pos );

        from->fwd = to->pos - from->pos;
        from->length = from->fwd.NormalizeInPlace();
    }

    // Update the currently last link.
    NavLink_t* prev = &m_Links[i-1];
    prev->fwd = vecGoal - prev->pos;
    prev->length = prev->fwd.NormalizeInPlace();

    // Set forward dot of every link.
    for ( i = 1; i < m_nLinkCount; i++ )
    {
        NavLink_t* from = &m_Links[i-1];
        NavLink_t* to = &m_Links[i];

        to->fwd_dot = from->fwd.Dot( to->fwd );

        // Completely redundant link.
        //if ( to->fwd_dot < -0.95f )
        //{
        //    int movecount = m_nLinkCount - i;
        //    memmove( from, to, movecount * sizeof(*m_Links) );
        //    --m_nLinkCount;
        //}
    }
    

    // Setup the last link as the actual goal position.
    // The link before is the position to the EDGE of the area.
    // We insert this link to actually get to the wanted position inside that area.
    ++m_nLinkCount;


    
    NavLink_t* last = &m_Links[i];

    last->area = pLastArea;
    last->fwd = prev->fwd;
    last->fwd_dot = 1.0f;
    last->length = 0.0f;
    last->navTraverse = prev->navTraverse;
    last->navTravel = TRAVEL_ONGROUND; // Assume we'll be traveling on ground.

    prev->area->ComputePortal( last->area, (NavDirType)last->navTraverse, &last->portalCenter, &last->portalHalfWidth );

    if ( last->area->Contains( vecGoal ) )
    {
        last->pos = vecGoal;
        last->pos.z = last->area->GetZ( vecGoal );
    }
    else
    {
        // We want to go to the exact spot?
        // This is for moving towards objects that may not be on the nav mesh.
        if ( IsUsingExactGoal() )
        {
            last->pos = vecGoal;
        }
        else
        {
            last->area->GetClosestPointOnArea( vecGoal, &last->pos );
        }
        
    }

    prev->fwd = last->pos - prev->pos;
    prev->length = prev->fwd.NormalizeInPlace();


    // Check for any jumps we have to do.
    for ( i = 1; i < m_nLinkCount; i++ )
    {
        NavLink_t* from = &m_Links[i-1];
        NavLink_t* to = &m_Links[i];

        if ( from->area == to->area )
            continue;

        // Compute the height difference.
        float height = from->area->ComputeAdjacentConnectionHeightChange( to->area );
        if ( height == FLT_MAX )
            continue;


        const float flStepHeight = cost.GetStepHeight();

        if ( height > flStepHeight )
        {
            NavLink_t* jumpstart = from;
            

            // We need to insert the jump start link here.
            int movecount = m_nLinkCount - i;
            if ( (movecount + i) < MAX_PATH_LINKS ) // If we can't insert an empty space just use the previous one
            {
                jumpstart = to;

                memmove( &(m_Links[i+1]), &(m_Links[i]), movecount * sizeof(*m_Links) );
                to = &m_Links[i+1];

                // Jump over the nav jump link next iteration
                ++i; // Don't use i after this point

                ++m_nLinkCount;
            }
            
            to->area->GetClosestPointOnArea( to->pos, &to->pos );

            jumpstart->navTravel = TRAVEL_NAVJUMP;
            jumpstart->area = from->area;

            // Push back a bit so we're not touching a wall or something.
            Vector back = -from->fwd;
            back.z = 0.0f;
            back.NormalizeInPlace();

            jumpstart->pos = to->pos + back * 64.0f;
            // Make sure we are within our area after backing up
            // Also updates Z
            jumpstart->area->GetClosestPointOnArea( jumpstart->pos, &jumpstart->pos );
        }
        else if ( height < (-flStepHeight) && (i + 1) < m_nLinkCount )
        {
            // We're going down, check if there's a drastic drop
            const Vector down = Vector( 0.0f, 0.0f, -1.0f );


            NavLink_t* droplink = &m_Links[i+1];

            // We don't change directions much, it's all good.
            if ( droplink->fwd_dot > 0.4f )
                continue;

            Vector dir = droplink->pos - to->pos;
            dir.NormalizeInPlace();

            // Yep, we're going down fast, make this a drop down link
            if ( dir.Dot( down ) > 0.6f )
                to->navTravel = TRAVEL_DROPDOWN;
        }
    }



    OnPathCreated();

    return true;
}

bool NPCR::CBaseNavPath::BuildNavPath( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const CBasePathCost& cost )
{
    if ( !pStartArea || !pGoalArea )
    {
        // Attempt a straight path if we have no areas.
        return cost.CanBuildSimpleRoute( vecStart, vecGoal ) ? BuildSimplePath( vecStart, vecGoal, true ) : false;
    }


    if ( pStartArea == pGoalArea )
    {
        return BuildSimplePath( vecStart, vecGoal, pStartArea, pGoalArea );
    }

    // Does most of the heavy lifting and builds the actual path.
    CNavArea* closestArea = nullptr;
    bool bPathResult = NavAreaBuildPath( pStartArea, pGoalArea, &vecGoal, cost, &closestArea, cost.GetMaxPathLength() );

    if ( !closestArea )
        return false;


    // Get count of segments we're going to go through.
    int count = 0;

    CNavArea* area;
    for ( area = closestArea; area; area = area->GetParent() )
    {
        ++count;

        if ( area == pStartArea )
            break;
    }


    if ( !ComputeNavPathDetails( count, vecStart, closestArea, vecGoal, cost ) )
        return false;


    return bPathResult;
}

// Builds a very basic ai graph path.
bool NPCR::CBaseNavPath::BuildGraphPath( const Vector& vecStart, const Vector& vecGoal, const CBasePathCost& cost )
{
    int startID = FindAINodeOfPosition( vecStart );
    int endID = FindAINodeOfPosition( vecGoal );

    // Just build a straight path.
    if ( startID == endID )
    {
        return BuildSimplePath( vecStart, vecGoal, true );
    }


    AI_Waypoint_t* first = FindGraphPath( startID, endID, cost );

    if ( !first )
    {
        // Couldn't build node path, attempt straight one.
        return cost.CanBuildSimpleRoute( vecStart, vecGoal ) ? BuildSimplePath( vecStart, vecGoal, true ) : false;
    }

    // Init links
    m_nLinkCount = 0;
    m_Links[0].fwd_dot = 1.0f;


    // Translate waypoints to links.
    
    int i = 0;
    AI_Waypoint_t* cur = first;
    do
    {
        // Ground only supported for now
        Assert( cur->NavType() <= NAV_JUMP );

        m_Links[i].area = nullptr;
        m_Links[i].pos = cur->GetPos();
        m_Links[i].navTravel = cur->NavType() != NAV_JUMP ? TRAVEL_ONGROUND : TRAVEL_NAVJUMP;
        //m_Links[i].navTraverse = GO_NORTH;
        

        ++i;
        ++m_nLinkCount;

        // We need one free extra space for the actual goal pos.
        if ( m_nLinkCount >= (MAX_PATH_LINKS-1) ) 
            break;
    }
    while ( (cur = cur->GetNext()) != nullptr );


    // Delete all waypoints. We don't need them anymore.
    DeleteAll( first );


    // Add last 
    ++m_nLinkCount;
    int last = m_nLinkCount - 1;
    m_Links[last].area = nullptr;
    m_Links[last].pos = vecGoal;


    // Set links' vectors.
    int prev = 0;
    for ( int cur = 1; cur < m_nLinkCount; cur++ )
    {
        prev = cur - 1;
        m_Links[prev].fwd = m_Links[cur].pos - m_Links[prev].pos;
        m_Links[prev].length = m_Links[prev].fwd.NormalizeInPlace();
    }

    m_Links[last].fwd = m_Links[last].pos - m_Links[prev].pos;
    m_Links[last].length = m_Links[last].fwd.NormalizeInPlace();




    OnPathCreated();

    return true;
}

bool NPCR::CBaseNavPath::CheckSimpleLOS( const Vector& vecStart, const Vector& vecEnd, trace_t& tr ) const
{
    CTraceFilterWorldOnly filter;
    UTIL_TraceLine(
        vecStart,
        vecEnd,
        MASK_OPAQUE, &filter, &tr );

    return tr.fraction == 1.0f;
}

bool NPCR::CBaseNavPath::ShouldDraw()
{
    return npcr_debug_path.GetBool();
}

void NPCR::CBaseNavPath::Draw()
{
    if ( !IsValid() )
        return;


    NavLink_t* lastlink = nullptr;
    for ( int i = 1; i < LinkCount(); i++ )
    {
        NavLink_t* from = &m_Links[i-1];
        NavLink_t* to = &m_Links[i];


        DrawSegment( from, to );


        lastlink = to;
    }

    DrawSegment( lastlink, LastLink() );
}

void NPCR::CBaseNavPath::DrawSegment( const NPCR::NavLink_t* from, const NPCR::NavLink_t* to )
{
    int r, g, b;
    switch ( from->navTravel )
    {
    case TRAVEL_NAVJUMP :   r = g = 0; b = 255; break;
    case TRAVEL_DROPDOWN :  r = g = 255; b = 0; break;
    case TRAVEL_ONGROUND :
    default :               r = g = b = 255; break;
    }

    NDebugOverlay::Line( from->pos, to->pos, r, g, b, true, 0.1f );
    NDebugOverlay::Box( from->pos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), r, g, b, 20, 0.1f );

    NDebugOverlay::Text( from->pos, UTIL_VarArgs( "%i", GetLinkOffset( from ) + 1 ), true, 0.1f );
}

const NPCR::NavLink_t* NPCR::CBaseNavPath::FirstLink() const
{
    return LinkCount() ? &m_Links[0] : nullptr;
}

const NPCR::NavLink_t* NPCR::CBaseNavPath::PreviousLink( const NPCR::NavLink_t* link ) const
{
    if ( !link )
        return nullptr;

    int i = GetLinkOffset( link ) - 1;
    if ( i < 0 || i >= LinkCount() )
        return nullptr;

    return &m_Links[i];
}

const NPCR::NavLink_t* NPCR::CBaseNavPath::NextLink( const NPCR::NavLink_t* link ) const
{
    if ( !link )
        return nullptr;

    int i = GetLinkOffset( link ) + 1;
    if ( i <= 0 || i >= LinkCount() )
        return nullptr;

    return &m_Links[i];
}

const NPCR::NavLink_t* NPCR::CBaseNavPath::LastLink() const
{
    return LinkCount() ? &m_Links[LinkCount()-1] : nullptr;
}

int NPCR::CBaseNavPath::GetLinkOffset( const NPCR::NavLink_t* link ) const
{
    if ( !link )
        return -1;

    return link - m_Links;
}
