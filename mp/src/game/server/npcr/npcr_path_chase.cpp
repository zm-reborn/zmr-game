#include "cbase.h"

#include "npcr_path_chase.h"


void NPCR::CChaseNavPath::Update( CBaseNPC* pNPC, CBaseCombatCharacter* pTarget, const CBasePathCost& cost )
{
    if ( NeedsRepath( pNPC, pTarget ) )
    {
        Compute( pNPC->GetCharacter(), pTarget, cost );
    }

    CFollowNavPath::Update( pNPC );
}

void NPCR::CChaseNavPath::Update( CBaseNPC* pNPC, CBaseEntity* pTarget, const CBasePathCost& cost )
{
    if ( NeedsRepath( pNPC, pTarget ) )
    {
        Compute( pNPC->GetCharacter(), pTarget, cost );
    }

    CFollowNavPath::Update( pNPC );
}

bool NPCR::CChaseNavPath::Compute( CBaseCombatCharacter* pNPC, CBaseEntity* pTarget, const CBasePathCost& cost, float maxDistanceToArea )
{
    Invalidate();


    const Vector vecStart = pNPC->WorldSpaceCenter();
    CNavArea* pStartArea = pNPC->GetLastKnownArea();
    const Vector vecGoal = pTarget->WorldSpaceCenter();
    CNavArea* pGoalArea;

    if ( pTarget->IsCombatCharacter() && pTarget->IsMoving() )
    {
        // Only predict position within this range.
        const float flPredictionStartDist = 64.0f;
        const float flPredictionEndDist = 1024.0f;

        float flDistToTargetSqr = vecStart.DistToSqr( vecGoal );

        if ( flDistToTargetSqr > ( flPredictionStartDist * flPredictionStartDist ) && flDistToTargetSqr < ( flPredictionEndDist * flPredictionEndDist ) )
        {
            Vector dir = pTarget->GetAbsVelocity();
            auto spd = dir.NormalizeInPlace();

            const Vector vecPredictedPosition = vecGoal + dir * MIN( 100.0f, spd );

            pGoalArea = TheNavMesh->GetNearestNavArea( vecPredictedPosition, true, maxDistanceToArea, true );
            if ( pGoalArea )
            {
                return BaseClass::Compute( vecStart, vecPredictedPosition, pStartArea, pGoalArea, cost );
            }
        }
    }

    pGoalArea = TheNavMesh->GetNearestNavArea( vecGoal, true, maxDistanceToArea, true );
    return BaseClass::Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
}

bool NPCR::CChaseNavPath::NeedsRepath( CBaseNPC* pNPC, CBaseEntity* pTarget )
{
    const NavLink_t* last = LastLink();
    if ( !last )
    {
        return true;
    }

    Vector vecEnemy = pTarget->GetAbsOrigin();
    Vector vecToEnemy = vecEnemy - pNPC->GetPosition();

    float distToGoal = (vecEnemy - last->pos).Length();
    float distToEnemy = vecToEnemy.Length();

    const float frac = 0.3f;
    float tolerance = distToEnemy * frac;

    return distToGoal > tolerance;
}
