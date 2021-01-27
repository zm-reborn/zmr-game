#pragma once

#include "npcs/zmr_playerbot.h"

#include "zmr_bot_main.h"
#include "zmr_survivor_attack.h"

#include "npcr_schedule.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"


//
// Close range version of an attack.
// Makes sure bot doesn't get too close to zombies.
//
class CSurvivorAttackCloseRangeSchedule : public CBasePlayerBotSchedule, public CSurvivorAttackInt
{
public:
    CSurvivorAttackCloseRangeSchedule();
    ~CSurvivorAttackCloseRangeSchedule();

    // Melee interface
    void SetMeleeing( bool value ) { m_bAllowMelee = value; }
    bool IsMeleeing() const { return m_bAllowMelee; }


    virtual const char* GetName() const OVERRIDE { return "SurvivorAttackCloseRange"; }


    virtual void OnStart() OVERRIDE;

    virtual void OnContinue() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE;

private:
    bool ShouldMoveInRange() const;

    bool IsInRangeToAttack( CBaseEntity* pEnemy ) const;

    bool ShouldMoveBack( CBaseEntity* pEnemy ) const;

    void MoveToShootingRange( CBaseEntity* pEnemy );

    void MoveBackFromThreat( CBaseEntity* pEnemy );


    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    bool m_bMovingToRange;
    bool m_bMovingOutOfRange;
    CountdownTimer m_NextRangeCheck;
    CountdownTimer m_NextMovingToRange;

    bool m_bAllowMelee;
};
