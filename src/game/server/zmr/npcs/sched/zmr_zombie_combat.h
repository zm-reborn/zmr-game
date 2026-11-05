#pragma once

#include "soundent.h"

#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_zombie_chase.h"


extern ConVar zm_sv_zombie_threat_investigation_dist;

//
// Monitors zombie's combat state.
// Attacks if an enemy is found, etc.
//
class CZombieCombatSchedule : public CBaseZombieSchedule
{
public:
    CZombieCombatSchedule();
    ~CZombieCombatSchedule();

    virtual const char* GetName() const override { return "ZombieCombat"; }

    virtual void OnStart() override;

    virtual void OnContinue() override;

    virtual void OnUpdate() override;

    virtual void OnAcquiredEnemy( CBaseEntity* pEnt ) override;

    virtual void OnSightGained( CBaseEntity* pEnt ) override;

    virtual void OnDamaged( const CTakeDamageInfo& info ) override;

    virtual void OnHeardSound( CSound* pSound ) override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

private:
    bool ShouldCareAboutThreat() const;

    bool GotoThreatPosition( const Vector& vecEnd );

    CBaseEntity* EvaluateEnemies();


    CZombieChaseSchedule* m_pChaseSched;

    Vector m_vecFaceTowards;
    CountdownTimer m_FaceTimer;
    CountdownTimer m_NextThreat;


    CountdownTimer m_NextEnemyScan;

    // For checking a potential threat
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_NextMove;


    CHandle<CBaseEntity> m_hLastEnemy;
    float m_flLastAlertSoundTime;
};
