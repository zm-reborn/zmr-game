#pragma once

#include "soundent.h"

#include "npcs/zmr_playerbot.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_bot_main.h"
#include "zmr_survivor_attack.h"
#include "zmr_survivor_attack_closerange.h"
#include "zmr_survivor_attack_longrange.h"


//
// Monitors bot's combat state.
// Attacks an enemy if found, etc.
//
class CSurvivorCombatSchedule : public CBasePlayerBotSchedule
{
public:
    CSurvivorCombatSchedule();
    ~CSurvivorCombatSchedule();

    virtual const char* GetName() const override { return "SurvivorCombat"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() override;

    virtual void OnStart() override;

    virtual void OnContinue() override;

    virtual void OnIntercept() override;

    virtual void OnUpdate() override;

    virtual void OnSpawn() override;

    virtual void OnHeardSound( CSound* pSound ) override;

    virtual void OnForcedMove( CNavArea* pArea ) override;

    virtual NPCR::QueryResult_t IsBusy() const override;

private:
    bool ShouldMoveBack( CBaseEntity* pEnemy ) const;

    void MoveBackFromThreat( CBaseEntity* pEnemy );


    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_NextEnemyScan;
    CountdownTimer m_NextRangeCheck;

    CountdownTimer m_NextHeardLook;
    CountdownTimer m_NextLookAround;
    CNavArea* m_pLastLookArea;
    Vector m_vecLookAt;

    bool m_bMovingOutOfRange;
    CHandle<CBaseEntity> m_hLastCombatTarget;

    CSurvivorAttackCloseRangeSchedule* m_pAttackCloseRangeSched;
    CSurvivorAttackLongRangeSchedule* m_pAttackLongRangeSched;
};
