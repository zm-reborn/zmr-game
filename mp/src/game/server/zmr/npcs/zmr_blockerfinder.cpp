#include "cbase.h"

#include "zmr_zombiebase.h"
#include "zmr_blockerfinder.h"


ConVar npcr_debug_zm_blocker( "npcr_debug_zm_blocker", "0" );


#define HULL_SIZE               16.0f
#define CHECK_DISTANCE          24.0f

CZMBlockerScanner::CZMBlockerScanner( CZMBaseZombie* pNPC )
    : NPCR::CEventListener( pNPC, pNPC ) // We need to be updated.
{
    m_nTimesBlocked = 0;
    m_hBlocker.Set( nullptr );

    m_NextBlockerCheck.Start( 0.1f );
    //m_NextSwatObjCheck.Start( 0.1f );
}

CZMBlockerScanner::~CZMBlockerScanner()
{

}

bool CZMBlockerScanner::ShouldUpdateBlocker() const
{
    return m_NextBlockerCheck.IsElapsed();
}

//bool CZMBlockerScanner::ShouldUpdateSwatting() const
//{
//    return m_NextSwatObjCheck.IsElapsed();
//}

void CZMBlockerScanner::Update()
{
    if ( ShouldUpdateBlocker() )
    {
        if ( GetNPC()->GetMotor()->IsMoving() )
        {
            FindBlocker();
        }
        else
        {
            m_nTimesBlocked = 0;
        }
        

        m_NextBlockerCheck.Start( 0.4f );
        return;
    }

    /*
    if ( ShouldUpdateSwatting() )
    {
        if ( GetNPC()->GetMotor()->GetVelocity().IsLengthGreaterThan( 1.0f ) )
        {
            FindBlocker();
        }

        m_NextSwatObjCheck.Start( 0.4f );
        return;
    }
    */
}

void CZMBlockerScanner::FindBlocker()
{
    Vector dir;
    CZMBaseZombie* pChar = GetOuter();

    NPCR::CFollowNavPath* path = pChar->GetCurrentPath();
    if ( path && path->GetGoalLink() )
    {
        dir = path->GetGoalLink()->pos - pChar->GetAbsOrigin();
        dir.z = 0.0f;
        dir.NormalizeInPlace();
    }
    else
    {
        AngleVectors( pChar->GetAbsAngles(), &dir );
    }


    float offset = pChar->GetMotor()->GetStepHeight();
    float height = (pChar->GetMotor()->GetHullHeight() - offset) / 2.0f;
    float hullsize = pChar->GetMotor()->GetHullWidth();
    
    Vector mins, maxs;
    Vector vecStart, vecEnd;
    trace_t tr;
        

    vecStart = pChar->GetAbsOrigin() + dir * hullsize / 2.0f;
    vecStart.z += height + offset;
    vecEnd = vecStart + dir * CHECK_DISTANCE;


    mins.x = mins.y =  -(hullsize / 2.0f);
    maxs.x = maxs.y = (hullsize / 2.0f);
    mins.z = -height;
    maxs.z = height;

        
    UTIL_TraceHull( vecStart, vecEnd, mins, maxs, MASK_NPCSOLID, pChar, COLLISION_GROUP_NONE, &tr );
    bool bHit = tr.DidHit() && tr.m_pEnt != nullptr;


    const bool bIsDebugging = npcr_debug_zm_blocker.GetBool();

    if ( bIsDebugging )
    {
        NDebugOverlay::SweptBox( vecStart, vecEnd, mins, maxs, QAngle( 0, 0, 0 ), bHit ? 255 : 0, (!bHit) ? 255 : 0, 0, 255, 0.1f );
    }


    if ( bHit )
    {
        if ( GetBlocker() == tr.m_pEnt )
        {
            m_nTimesBlocked++;

            if ( bIsDebugging )
                DevMsg( "Same blocker %i times!\n", GetTimesBlocked() );
        }
        else
        {
            m_nTimesBlocked = 1;
        }
        
        m_hBlocker.Set( tr.m_pEnt );
    }
    else
    {
        m_hBlocker.Set( nullptr );
    }
}
