#include "cbase.h"

#include "npcr_motor.h"
#include "npcr_path_follow.h"



ConVar npcr_debug_path_avoid( "npcr_debug_path_avoid", "0" );
ConVar npcr_debug_path_stuck( "npcr_debug_path_stuck", "0" );


// 80 degrees
#define STUCK_ANGLE_START       1.39626f

class CAvoidTraceFilter : public CTraceFilterSimple
{
public:
    CAvoidTraceFilter( NPCR::CBaseNPC* pNPC, int collisionGroup ) : CTraceFilterSimple( pNPC->GetCharacter(), collisionGroup )
    {
        m_pNPC = pNPC;

        m_vecMyDir = m_pNPC->GetVel().Normalized();
        m_pRootParent = pNPC->GetCharacter()->GetRootMoveParent();
    }

    virtual bool ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask ) OVERRIDE
    {
        if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
        {
            CBaseEntity* pEnt = EntityFromEntityHandle( pServerEntity );
            if ( !pEnt )
                return false;

            // We may have something parented to us, as per usual for silly maps to do. Ignore em.
            if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pEnt ) )
                return false;

            // Ignore the enemy, we're suppose to attack them!
            if ( m_pNPC->IsTargetedEnemy( pEnt ) )
                return false;


            NPCR::QueryResult_t res = m_pNPC->ShouldTouch( pEnt );
            if ( res == NPCR::RES_YES )
                return true;
            if ( res == NPCR::RES_NO )
                return false;
            
            if ( pEnt->MyCombatCharacterPointer() )
            {
                Vector vel = pEnt->GetLocalVelocity();
                float spd = vel.NormalizeInPlace();

                // They're not moving anywhere, go around.
                if ( spd < 10.0f )
                    return true;

                // Slowpokes
                if ( spd < (m_pNPC->GetMotor()->GetMovementSpeed() * 0.7f) )
                    return true;

                // If we're going opposite directions, we need to go around them.
                if ( m_vecMyDir.Dot( vel ) < -0.4f )
                    return true;

                // We're probably moving WITH them, so just trust them to move out of our way.
                return false;
            }

            return true;
        }

        return false;
    }

private:
    NPCR::CBaseNPC* m_pNPC;
    Vector m_vecMyDir;
    CBaseEntity* m_pRootParent;
};

void NPCR::CFollowNavPath::Invalidate()
{
    CBaseNavPath::Invalidate();

    m_pGoal = nullptr;
    m_pCurLink = nullptr;


    m_bStuckPosCheck = false;
    m_nStuckTimes = 0;
    m_StuckTimer.Invalidate();
    m_flStuckAngle = STUCK_ANGLE_START;
    m_StuckCheckTimer.Invalidate();


    m_JumpStatus = JUMP_BEGIN;
}

NPCR::PathRes_t NPCR::CFollowNavPath::CheckGoal( CBaseNPC* pNPC ) const
{
    if ( !m_pGoal )
        return PATH_OK;


    Vector vecMyPos = pNPC->GetPosition();

    if ( m_pCurLink && m_pCurLink->navTravel == TRAVEL_NAVJUMP )
    {
        if ( ShouldFailNavJump( pNPC ) )
            return PATH_FAILED;
    }



    const NavLink_t* next = NextLink( m_pGoal );

    const float flGoalToGoalTolerance = 48.0f;
    float tolerance = next ? GetMoveTolerance() : GetGoalTolerance();

    // Decrease tolerance if our previous link is REALLY close to ours and we have a tight curve ahead of us.
    // This stops NPCs getting stuck on doorways because of slim frame and tight corners.
    const NavLink_t* prev = PreviousLink( m_pGoal );
    if ( prev && prev->length < flGoalToGoalTolerance && m_pGoal->fwd_dot < 0.7f )
    {
        tolerance = 8.0f;
    }
    else if ( m_pGoal->navTravel == TRAVEL_NAVJUMP )
    {
        // Decrease tolerance for nav jump links to make sure we don't hit walls
        tolerance = 8.0f;
    }


    Vector dirToMe = vecMyPos - m_pGoal->pos;
    float distToGoalSqr = dirToMe.Length2DSqr();

    // We close enough to goal?
    if ( distToGoalSqr < (tolerance*tolerance) )
    {
        return PATH_SUCCESS;
    }


    // Check the next link in case current goal is redundant.
    if ( m_pGoal->navTravel == TRAVEL_ONGROUND && next && distToGoalSqr < (128.0f*128.0f) )
    {
        // It's pretty much just a straight line to the next one, we can ignore this one then.
        if ( m_pGoal->fwd_dot > 0.9f )
            return PATH_SUCCESS;


        dirToMe.NormalizeInPlace();

        // We're literally on the path to the next goal.
        if ( m_pGoal->fwd.Dot( dirToMe ) > 0.8f )
            return PATH_SUCCESS;

        // Only if it isn't terribly curved.
        if ( m_pGoal->fwd_dot > 0.6f )
        {
            // We're closer to the next link
            float distToNextSqr = next->pos.DistToSqr( vecMyPos );
            float goalToNextSqr = m_pGoal->length;
            goalToNextSqr *= goalToNextSqr;
            if ( distToNextSqr < distToGoalSqr || distToNextSqr < goalToNextSqr )
            {
                return PATH_SUCCESS;
            }
        }
    }

    return PATH_OK;
}

bool NPCR::CFollowNavPath::ShouldFailNavJump( CBaseNPC* pNPC ) const
{
    if ( m_JumpStatus == JUMP_BEGIN )
        return false;


    bool bOnGround = pNPC->GetMotor()->IsOnGround();
        
    // We're not on groud yet.
    if ( !bOnGround )
        return false;


    // We seemingly failed our jump.
    if ( (m_pGoal->pos.z-8.0f) > pNPC->GetPosition().z )
    {
        return true;
    }

    return false;
}

void NPCR::CFollowNavPath::FollowPath( CBaseNPC* pNPC )
{
    // Check if we've reached the goal.
    PathRes_t res = PATH_OK;
    while ( m_pGoal )
    {
        res = CheckGoal( pNPC );
        if ( res != PATH_SUCCESS )
            break;


        ClearAvoidState();
        m_pCurLink = m_pGoal;
        m_pGoal = NextLink( m_pGoal );

        if ( m_pGoal )
        {
            OnNewGoal( pNPC );
        }
    }

    // No more goal, we must've finished.
    if ( res == PATH_SUCCESS )
    {
        OnPathSuccess();
        pNPC->OnMoveSuccess( this );
        return;
    }
    else if ( res == PATH_FAILED )
    {
        OnPathFailed();
        pNPC->OnMoveFailed( this );
        return;
    }


    UpdateMove( pNPC );
}

void NPCR::CFollowNavPath::UpdateMove( CBaseNPC* pNPC )
{
    GroundMove( pNPC );
}

void NPCR::CFollowNavPath::GroundMove( CBaseNPC* pNPC )
{
    if ( !pNPC->GetMotor()->IsOnGround() )
        return;


    Vector vecGoal = m_pGoal->pos;


    int iStuck = CheckStuck( pNPC, vecGoal );
    bool bIsDoingStuckMove = iStuck != 0;


    int res = CheckAvoid( pNPC, vecGoal );

    if ( !bIsDoingStuckMove )
    {
        // Periodically check if we're stuck even if we're not currently trying to avoid anything.
        // It's possible that our avoid traces don't hit anything, but we're still stuck.
        if ( !m_StuckCheckTimer.HasStarted() || m_StuckCheckTimer.IsElapsed() )
        {
            StartCheckingStuck( pNPC );

            m_StuckCheckTimer.Start( 2.0f );
        }
        else if ( res == 1 )
        {
            // We started avoiding an obstacle, start checking our position whether we're going anywhere or not.
            StartCheckingStuck( pNPC );
        }
        else
        {
            // Nothing in our way.
            if ( m_bStuckPosCheck && m_nStuckTimes <= 0 )
            {
                StopCheckingStuck();
            }
        }
    }


    // Keep moving towards current goal
    pNPC->GetMotor()->Approach( vecGoal );
}

void NPCR::CFollowNavPath::Update( CBaseNPC* pNPC )
{
    if ( m_pGoal )
    {
        FollowPath( pNPC );
    }

    if ( ShouldDraw() )
    {
        Draw();
    }
}

void NPCR::CFollowNavPath::OnNewGoal( CBaseNPC* pNPC )
{
    m_JumpStatus = JUMP_BEGIN;

    // Start nav jump
    if ( GetCurLink() && GetCurLink()->navTravel == TRAVEL_NAVJUMP )
    {
        pNPC->GetMotor()->NavJump( GetGoalLink()->pos );
        m_JumpStatus = JUMP_IN_AIR;
    }
}

void NPCR::CFollowNavPath::Draw()
{
    CBaseNavPath::Draw();

    if ( IsValid() && m_pGoal )
    {
        bool bIsGoal = NextLink( m_pGoal ) == nullptr;
        NDebugOverlay::Circle( m_pGoal->pos, MAX( 1.0f, bIsGoal ? GetGoalTolerance() : GetMoveTolerance() ), 255, 0, 0, 255, true, 0.1f );
    }
}

bool NPCR::CFollowNavPath::ShouldDecelerateToGoal() const
{
    if ( IsValid() && GetGoalLink() == LastLink() )
    {
        return true;
    }

    return false;
}

int NPCR::CFollowNavPath::CheckAvoid( CBaseNPC* pNPC, Vector& vecGoalPos )
{
    VPROF_BUDGET( "CFollowNavPath::CheckAvoid", "NPCR" );


    Vector vecMyPos = pNPC->GetPosition();

    // We're currently doing the avoiding.
    // Just move the goal.
    if ( m_AvoidTimer.HasStarted() && !m_AvoidTimer.IsElapsed() )
    {
        vecGoalPos = vecMyPos + m_vecAvoidDelta;
        return -1;
    }

    // Wait for next check
    if ( m_NextAvoidCheck.HasStarted() && !m_NextAvoidCheck.IsElapsed() )
        return -1;


    // Trace left and right
    // If one trace is longer, pick that side and strafe that way.
    m_NextAvoidCheck.Start( 0.4f );

    Vector fwd = (vecGoalPos - vecMyPos);
    fwd.z = 0.0f;
    float distToGoal = fwd.NormalizeInPlace();

    // No point avoiding within this distance.
    if ( distToGoal <= GetAvoidMinDistance() )
        return -1;


    Vector right( fwd.y, -fwd.x, 0.0f );



    float realWidth = pNPC->GetMotor()->GetHullWidth();

    float hullWidth = pNPC->GetMotor()->GetHullWidth() / 4.0f;
    float offset = hullWidth;

    Vector scanMins = Vector( -hullWidth, -hullWidth, pNPC->GetMotor()->GetStepHeight() );
    Vector scanMaxs = Vector( hullWidth, hullWidth, pNPC->GetMotor()->GetHullHeight() );

    float tolerance = GetGoalTolerance() + realWidth;
    float length = 30.0f;

    if ( (distToGoal - tolerance) < 0.0f )
    {
        length = MAX( realWidth, distToGoal - realWidth );
    }


    const bool bIsDebugging = npcr_debug_path_avoid.GetBool();
    
    Vector scanStart = vecMyPos;
    Vector scanRight = scanStart + right * offset;
    Vector scanLeft = scanStart + -right * offset;

    Vector scanRightEnd = scanRight + fwd * length;
    Vector scanLeftEnd = scanLeft + fwd * length;

    float rightFrac, leftFrac;
    Vector rightNormal, leftNormal;

    trace_t tr;

    CAvoidTraceFilter filter( pNPC, COLLISION_GROUP_NPC );


    // Trace right
    UTIL_TraceHull( scanRight, scanRightEnd, scanMins, scanMaxs, MASK_NPCSOLID, &filter, &tr );

    rightFrac = tr.startsolid ? 0.0f : tr.fraction; // Startsolid != frac 0.0
    rightNormal = tr.startsolid ? -fwd : tr.plane.normal;
    // Nothing in way
    const bool rightClear = rightFrac == 1.0f;

    if ( bIsDebugging )
    {
        NDebugOverlay::SweptBox( scanRight, scanRightEnd, scanMins, scanMaxs, vec3_angle, rightClear ? 0 : 255, rightClear ? 255 : 0, 0, 255, 0.1f );


        if ( !rightClear && !tr.startsolid )
        {
            QAngle ang; VectorAngles( tr.plane.normal, ang );
            NDebugOverlay::Axis( tr.endpos, ang, 16.0f, true, 1.0f );
        }
    }

    scanRightEnd = tr.endpos;


    // Trace left
    UTIL_TraceHull( scanLeft, scanLeftEnd, scanMins, scanMaxs, MASK_NPCSOLID, &filter, &tr );

    leftFrac = tr.startsolid ? 0.0f : tr.fraction; // Startsolid != frac 0.0
    leftNormal = tr.startsolid ? -fwd : tr.plane.normal;
    // Nothing in way
    const bool leftClear = leftFrac == 1.0f;

    if ( bIsDebugging )
    {
        NDebugOverlay::SweptBox( scanLeft, scanLeftEnd, scanMins, scanMaxs, vec3_angle, leftClear ? 0 : 255, leftClear ? 255 : 0, 0, 255, 0.1f );


        if ( !leftClear && !tr.startsolid )
        {
            QAngle ang; VectorAngles( tr.plane.normal, ang );
            NDebugOverlay::Axis( tr.endpos, ang, 16.0f, true, 1.0f );
        }
    }

    scanLeftEnd = tr.endpos;


    // Nothing in our way
    if ( rightClear && leftClear )
    {
        return 0;
    }


    // Just a slope? We'll be fine.
    float minSlope = pNPC->GetMotor()->GetSlopeLimit();
    if ((rightClear || rightNormal.z > minSlope)
    &&  (leftClear || leftNormal.z > minSlope))
    {
        return 0;
    }

    // We hit something that is further than our goal. We'll be fine.
    if ((rightClear || distToGoal < (length*rightFrac))
    &&  (leftClear || distToGoal < (length*leftFrac)))
    {
        return 0;
    }


    Vector normal;

    float side = 0.0f;
    if ( !rightClear && !leftClear )
    {
        // Both not clear, pick the one that is further away.

        
        const float epsilon = 0.001f;
        float delta = leftFrac - rightFrac;

        if ( abs( delta ) > epsilon )
        {
            side = delta > 0.0f ? -1.0f : 1.0f;
            normal = delta > 0.0f ? leftNormal : rightNormal;
        }
    }
    else
    {
        // Can't really do anything here then.
        if ((leftClear && rightFrac == 0.0f)
        ||  (rightClear && leftFrac == 0.0f))
        {
            return 0;
        }


        side = leftClear ? -1.0f : 1.0f;
        normal = leftClear ? rightNormal : leftNormal;
    }

    if ( bIsDebugging )
    {
        const char* sidename = (side != 0.0f ? side : -m_flLastSide) > 0.0f ? "right" : "left";

        DevMsg( "[NPCR] Avoiding to the %s! (Left | Frac: %s%.1f) (Right | Frac: %s%.1f)%s\n",
            sidename,
            leftFrac == 0.0f ? "S" : "", leftFrac,
            rightFrac == 0.0f ? "S" : "", rightFrac,
            side == 0.0f ? " (Opposite side of last time)" : "" );
    }

    // Both sides are equally fucked, just pick the opposite of last time.
    if ( side == 0.0f )
    {
        side = -m_flLastSide;
        normal = -fwd;
    }

    normal.z = 0.0f;
    normal.NormalizeInPlace();

    
    float dot = abs( fwd.AsVector2D().Dot( normal.AsVector2D() ) );
    dot *= side;


    m_vecAvoidDelta = (vecGoalPos + right * MAX( 100.0f, distToGoal ) * dot) - vecMyPos;


    /*if ( m_nTimesAvoided )
    {
        // Add even more if our last attempt failed.
        newPos += right * 32.0f * m_nTimesAvoided * side;
    }*/


    m_flLastSide = side;

    // Avoid the same way for this long and check again later.
    m_AvoidTimer.Start( 0.4f );


    vecGoalPos = vecMyPos + m_vecAvoidDelta;


    if ( bIsDebugging )
    {
        NDebugOverlay::Box( vecGoalPos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), 255, 0, 0, 255, 0.4f );
        NDebugOverlay::Line( pNPC->GetCharacter()->WorldSpaceCenter(), vecGoalPos, 255, 255, 255, true, 0.4f );
    }

    return 1;
}

int NPCR::CFollowNavPath::CheckStuck( CBaseNPC* pNPC, Vector& vecGoalPos )
{
    bool bIsDoingStuckMove = m_StuckTimer.HasStarted() && !m_StuckTimer.IsElapsed();

    if ( bIsDoingStuckMove )
    {
        // Keep going that way.
        Vector vecMyPos = pNPC->GetPosition();
        vecGoalPos = vecMyPos + m_vecStuckDelta;

        return -1;
    }
    else if ( m_bStuckPosCheck )
    {
        // If we haven't moved, increase our stuck time.
        Vector vecMyPos = pNPC->GetPosition();
        if ( m_vecStuckPos.AsVector2D().DistToSqr( vecMyPos.AsVector2D() ) < (2.0f*2.0f) )
        {
            ++m_nStuckTimes;
        }
        else
        {
            m_vecStuckPos = vecMyPos;

            --m_nStuckTimes;
            if ( m_nStuckTimes < 0 )
                m_nStuckTimes = 0;
        }


        float timeStuck = m_nStuckTimes * pNPC->GetUpdateInterval();

        if ( timeStuck > 0.4f )
        {
            // We've been stuck for a while.


            if ( npcr_debug_path_stuck.GetBool() )
            {
                DevMsg( "We're stuck! Starting to do stuck move.\n" );
            }

            // Go back a bit
            const Vector vecMyPos = pNPC->GetPosition();
            Vector vecBack = vecMyPos - vecGoalPos;
            vecBack.z = 0.0f;
            vecBack.NormalizeInPlace();

            // Pick the side that we were trying to move to last time
            float angle = atan2( vecBack.y, vecBack.x ) + (m_flLastSide * m_flStuckAngle);

            vecBack.x = cos( angle );
            vecBack.y = sin( angle );


            m_vecStuckDelta = vecBack * 100.0f;

            // Keep decreasing the angle if we keep repeating this.
            m_flStuckAngle -= 0.3f;
            if ( m_flStuckAngle < 0.0f )
                m_flStuckAngle = STUCK_ANGLE_START;

            m_StuckTimer.Start( 1.0f );
            m_nStuckTimes = 0;
            m_bStuckPosCheck = false;

            return 1;
        }
    }

    return 0;
}

void NPCR::CFollowNavPath::StartCheckingStuck( NPCR::CBaseNPC* pNPC )
{
    if ( !m_bStuckPosCheck && npcr_debug_path_stuck.GetBool() )
    {
        DevMsg( "Started checking for stuck\n" );
    }

    m_bStuckPosCheck = true;
    m_vecStuckPos = pNPC->GetPosition();
}

void NPCR::CFollowNavPath::StopCheckingStuck()
{
    if ( m_bStuckPosCheck && npcr_debug_path_stuck.GetBool() )
    {
        DevMsg( "Stopped checking for stuck\n" );
    }

    m_bStuckPosCheck = false;
    m_nStuckTimes = 0;
}

