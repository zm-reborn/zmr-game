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
