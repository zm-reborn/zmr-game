#pragma once

#include "npcs/zmr_zombiebase.h"


typedef NPCR::CSchedule<CZMBaseZombie> CBaseZombieSchedule;



#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

extern ConVar zm_sv_defense_chase_dist;
extern ConVar zm_sv_defense_goal_tolerance;
extern ConVar zm_sv_zombie_move_start_areafinddist;

class CZombieGotoSwatObjSchedule;


//
// Entrypoint to zombie schedules.
//
class CZombieMainSchedule : public CBaseZombieSchedule
{
public:
    virtual const char* GetName() const override { return "ZombieMain"; }

    virtual NPCR::CSchedule<CZMBaseZombie>* CreateFriendSchedule() override;

    virtual void OnStart() override;

    //virtual void OnEnd() override;

    virtual void OnUpdate() override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t type ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

    virtual void OnChase( CBaseEntity* pEnt ) override;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;

    // Wait for us to finish before doing something else.
    virtual NPCR::QueryResult_t IsBusy() const override;

private:
    CZombieMainSchedule();
    ~CZombieMainSchedule();


    bool Command( CZMPlayer* pCommander, const Vector& vecPos );
    bool CommandSwat( CZMPlayer* pCommander, CBaseEntity* pEnt, bool bBreak = true );


    bool ShouldDefendFrom( CBaseEntity* pEnemy ) const;

    const Vector& GetGoalPos();

    void UpdateGoal( const Vector& vecPos );


    NPCR::CFollowNavPath* m_pPath;
    NPCR::CPathCostGroundOnly m_PathCost;
    Vector m_vecCurrentGoal;
    bool m_bHasGoal;
    CountdownTimer m_BlockerTimer;
    CZombieGotoSwatObjSchedule* m_pGotoSwatSched;


    friend class CZMBaseZombie;
};
