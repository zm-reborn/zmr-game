#pragma once

#include "npcr/npcr_path_follow.h"


class CZMBansheeFollowPath : public NPCR::CFollowNavPath
{
public:
    CZMBansheeFollowPath();

    typedef NPCR::CFollowNavPath BaseClass;
    typedef CZMBansheeFollowPath ThisClass;

    virtual bool Compute( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const NPCR::CBasePathCost& cost ) override;

    
    bool CanPotentiallyJumpTo( const Vector& vecStart, const Vector& vecGoal ) const;

    // Returns minimum jump height in order to jump over obstacles.
    float FindJumpHeight( const Vector& vecStart, const Vector& vecGoal ) const;

    virtual NPCR::PathRes_t CheckGoal( NPCR::CBaseNPC* pNPC ) const override;
    virtual void UpdateMove( NPCR::CBaseNPC* pNPC ) override;
    virtual void OnNewGoal( NPCR::CBaseNPC* pNPC ) override;

    virtual void Invalidate() override;

protected:
    float m_flHeight;
};
