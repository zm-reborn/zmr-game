#pragma once


#include "npcs/zmr_zombiebase.h"
#include "zmr_zombie_main.h"

//
// Performs the melee attack of zombies.
//
class CZombieClawAttackSched : public CBaseZombieSchedule
{
public:
    CZombieClawAttackSched();
    ~CZombieClawAttackSched();

    virtual const char* GetName() const OVERRIDE { return "ZombieClawAttack"; }


    virtual void OnStart() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    //virtual void OnAnimActivityFinished( Activity completedActivity ) OVERRIDE;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE;

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE;

    virtual void OnAttacked() OVERRIDE;

    virtual void OnEnd() OVERRIDE;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE;

private:
    CountdownTimer m_FinishTimer;
    Vector m_vecLastFacing;
    bool m_bDidAttack;
};
