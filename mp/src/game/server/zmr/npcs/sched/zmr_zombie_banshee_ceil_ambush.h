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
    virtual const char* GetName() const OVERRIDE { return "BansheeCeilingAmbush"; }

    virtual CZMBanshee* GetOuter() const OVERRIDE { return static_cast<CZMBanshee*>( CBaseZombieSchedule::GetOuter() ); }

    virtual void OnStart() OVERRIDE;

    virtual void OnEnd() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual void OnContinue() OVERRIDE;

    virtual void OnAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE;

    virtual void OnLandedGround( CBaseEntity* pEnt ) OVERRIDE;

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* tr ) OVERRIDE;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE;

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE;

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
