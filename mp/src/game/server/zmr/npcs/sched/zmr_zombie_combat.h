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

    virtual const char* GetName() const OVERRIDE { return "ZombieCombat"; }

    virtual void OnStart() OVERRIDE;

    virtual void OnContinue() OVERRIDE;

    virtual void OnUpdate() OVERRIDE;

    virtual void OnAcquiredEnemy( CBaseEntity* pEnt ) OVERRIDE;

    virtual void OnSightGained( CBaseEntity* pEnt ) OVERRIDE;

    virtual void OnDamaged( const CTakeDamageInfo& info ) OVERRIDE;

    virtual void OnHeardSound( CSound* pSound ) OVERRIDE;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE;

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
