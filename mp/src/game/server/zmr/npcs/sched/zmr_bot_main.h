#pragma once

#include "npcs/zmr_playerbot.h"

#include "npcr_schedule.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"



typedef NPCR::CSchedule<CZMPlayerBot> CBasePlayerBotSchedule;

//
// Entrypoint to player bot schedules.
//
class CPlayerBotMainSchedule : public CBasePlayerBotSchedule
{
public:
    CPlayerBotMainSchedule();
    ~CPlayerBotMainSchedule();

    virtual const char* GetName() const OVERRIDE { return "PlayerBotMain"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE;

    virtual void OnStart() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;
    

private:
    void ComputePath();

    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    CountdownTimer m_ExpireTimer;
};
