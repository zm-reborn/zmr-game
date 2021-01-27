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
    virtual const char* GetName() const OVERRIDE { return "BansheeLeap"; }

    virtual CZMBanshee* GetOuter() const OVERRIDE { return static_cast<CZMBanshee*>( CBaseZombieSchedule::GetOuter() ); }

    virtual void OnStart() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual void OnAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual void OnLandedGround( CBaseEntity* pGround ) OVERRIDE;

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* pTrace ) OVERRIDE;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE;

    virtual void OnAnimActivityFinished( Activity completedActivity ) OVERRIDE;

    virtual void OnEnd() OVERRIDE;

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE;

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
