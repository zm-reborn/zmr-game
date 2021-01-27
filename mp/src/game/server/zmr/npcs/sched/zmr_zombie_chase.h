#pragma once


#include "npcs/zmr_zombiebase.h"

#include "npcr_path_cost.h"
#include "npcr_path_chase.h"

#include "zmr_zombie_main.h"
#include "zmr_zombie_clawattack.h"
#include "zmr_zombie_swat_scan.h"
#include "zmr_zombie_swat_goto.h"

//
// Chases an enemy until close enough to attack or enemy is lost.
//
class CZombieChaseSchedule : public CBaseZombieSchedule
{
public:
    CZombieChaseSchedule();
    ~CZombieChaseSchedule();

    virtual const char* GetName() const OVERRIDE { return "ZombieChase"; }


    virtual void OnStart() OVERRIDE;

    virtual void OnContinue() OVERRIDE;

    virtual void OnEnd() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE;

    virtual void OnStuck() OVERRIDE;

private:
    CZombieClawAttackSched* m_pAttackSched;
    CZombieScanSwatObjSchedule* m_pScanSwatSched;
    CZombieGotoSwatObjSchedule* m_pGotoSwatSched;
    CountdownTimer m_SwatScanTimer;
    CountdownTimer m_BlockerTimer;
    CountdownTimer m_EnemyScanTimer;
    CountdownTimer m_StuckTimer;

    NPCR::CChaseNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
};
