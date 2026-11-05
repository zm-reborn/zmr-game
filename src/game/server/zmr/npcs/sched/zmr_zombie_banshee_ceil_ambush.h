#pragma once


#include "npcs/zmr_zombiebase.h"
#include "npcs/zmr_banshee.h"
#include "zmr_zombie_main.h"

//
// Performs ceiling ambush for banshees.
// If ambush is triggered, banshee will leap.
//
class CBansheeCeilAmbushSchedule : public CBaseZombieSchedule
{
public:
    CBansheeCeilAmbushSchedule();

    virtual const char* GetName() const override { return "BansheeCeilingAmbush"; }

    virtual CZMBanshee* GetOuter() const override { return static_cast<CZMBanshee*>( CBaseZombieSchedule::GetOuter() ); }

    virtual void OnStart() override;

    virtual void OnEnd() override;

    virtual void OnUpdate() override;

    virtual void OnContinue() override;

    virtual void OnAnimEvent( animevent_t* pEvent ) override;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) override;

    virtual void OnLandedGround( CBaseEntity* pEnt ) override;

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* tr ) override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

    virtual NPCR::QueryResult_t IsBusy() const override;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;

private:
    bool IsCeilingFlat( const Vector& plane ) const;

    CBaseEntity* GetClingAmbushTarget() const;

    void Leap( CBaseEntity* pTarget );


    CountdownTimer m_ExpireTimer;
    CountdownTimer m_NextAmbushCheck;
    bool m_bOnCeiling;
    bool m_bInLeap;
    Vector m_vecCeilingPos;
    Vector m_vecStartPos;
    float m_flLeapTowardsYaw;
    bool m_bDidLeapAttack;
};
