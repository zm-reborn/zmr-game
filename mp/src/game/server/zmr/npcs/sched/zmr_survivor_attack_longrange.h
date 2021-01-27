#pragma once

#include "npcs/zmr_playerbot.h"

#include "zmr_bot_main.h"
#include "zmr_survivor_attack.h"
#include "zmr_survivor_attack_closerange.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

//
// Long range version of an attack.
// Requires the bot to be more precise.
//
class CSurvivorAttackLongRangeSchedule : public CBasePlayerBotSchedule, public CSurvivorAttackInt
{
public:
    CSurvivorAttackLongRangeSchedule();
    ~CSurvivorAttackLongRangeSchedule();

    virtual const char* GetName() const OVERRIDE { return "SurvivorAttackLongRange"; }

    virtual void OnStart() OVERRIDE;

    virtual void OnContinue() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE;

private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    CSurvivorAttackCloseRangeSchedule* m_pAttackCloseRangeSched;
};
