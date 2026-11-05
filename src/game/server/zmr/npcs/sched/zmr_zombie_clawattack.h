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

    virtual const char* GetName() const override { return "ZombieClawAttack"; }


    virtual void OnStart() override;

    virtual void OnUpdate() override;

    //virtual void OnAnimActivityFinished( Activity completedActivity ) override;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) override;

    virtual NPCR::QueryResult_t IsBusy() const override;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;

    virtual void OnAttacked() override;

    virtual void OnEnd() override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

private:
    CountdownTimer m_FinishTimer;
    Vector m_vecLastFacing;
    bool m_bDidAttack;
};
