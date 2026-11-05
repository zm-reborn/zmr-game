#pragma once


#include "npcs/zmr_zombiebase.h"
#include "npcs/zmr_banshee.h"
#include "zmr_zombie_main.h"

extern ConVar zm_sk_banshee_dmg_leap;

//
// Performs the leap for the banshee.
//
class CBansheeLeapSchedule : public CBaseZombieSchedule
{
public:
    CBansheeLeapSchedule();

    virtual const char* GetName() const override { return "BansheeLeap"; }

    virtual CZMBanshee* GetOuter() const override { return static_cast<CZMBanshee*>( CBaseZombieSchedule::GetOuter() ); }

    virtual void OnStart() override;

    virtual void OnUpdate() override;

    virtual void OnAnimEvent( animevent_t* pEvent ) override;

    virtual void OnLandedGround( CBaseEntity* pGround ) override;

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* pTrace ) override;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) override;

    virtual void OnAnimActivityFinished( Activity completedActivity ) override;

    virtual void OnEnd() override;

    virtual NPCR::QueryResult_t IsBusy() const override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

private:
    void DoLeapStart();

    bool DoLeap();

    bool m_bInLeap;
    //
    // Don't do a wonky landing animation anymore.
    //
    //Activity m_iLandAct;
    bool m_bLanded;
    CountdownTimer m_FinishTimer;
    CountdownTimer m_ExpireTimer;
    bool m_bDidLeapAttack;
};
