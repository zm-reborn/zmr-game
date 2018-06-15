#pragma once

#include "npcr/npcr_path_follow.h"


class CZMBansheeFollowPath : public NPCR::CFollowNavPath
{
public:
    typedef NPCR::CFollowNavPath BaseClass;
    typedef CZMBansheeFollowPath ThisClass;

    virtual bool Compute( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const NPCR::CBasePathCost& cost ) OVERRIDE;

    
    bool CanPotentiallyJumpTo( const Vector& vecStart, const Vector& vecGoal ) const;

    // Returns minimum jump height in order to jump over obstacles.
    float FindJumpHeight( const Vector& vecStart, const Vector& vecGoal ) const;

    virtual void UpdateMove( NPCR::CBaseNPC* pNPC ) OVERRIDE;
    virtual void OnNewGoal( NPCR::CBaseNPC* pNPC ) OVERRIDE;

    virtual void Invalidate() OVERRIDE;

protected:
    float m_flHeight;
};
