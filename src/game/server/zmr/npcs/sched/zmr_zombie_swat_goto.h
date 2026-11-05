#pragma once

#include "npcs/zmr_zombiebase.h"

#include "npcr_path_cost.h"
#include "zmr_zombie_main.h"
#include "zmr_zombie_swat.h"


//
// Go to a swattable (or breakable) object.
// See the swatting schedule for the actual swat.
//
class CZombieGotoSwatObjSchedule : public CBaseZombieSchedule, public CSwatInt
{
public:
    CZombieGotoSwatObjSchedule();
    ~CZombieGotoSwatObjSchedule();

    //
    // Swat goto interface
    //
    void SetStartExpireTime( float time ) { m_flStartExpireTimer = time; }

    bool DoCheckDirection() const { return m_bCheckDirection; }
    void SetCheckDirection( bool state ) { m_bCheckDirection = state; }

    bool DoCheckForEnemies() const { return m_bCheckForEnemies; }
    void SetCheckForEnemies( bool state ) { m_bCheckForEnemies = state; }


    virtual const char* GetName() const override { return "ZombieGotoSwatObj"; }


    virtual void OnStart() override;

    virtual void OnContinue() override;

    virtual void OnUpdate() override;

    virtual void OnChase( CBaseEntity* pEnemy ) override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) override;


private:
    bool CheckObject( CBaseEntity* pSwat );

    bool IsCloseEnough( CBaseEntity* pSwat ) const;

    bool ComputeNewPath( const Vector& vecEnd );

    CZombieSwatObjSchedule* m_pSwatSched;
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_ExpireTimer;
    Vector m_vecStartSwatPos;
    Vector m_vecCurSwatPos;
    bool m_bCheckDirection;
    bool m_bCheckForEnemies; // Scan for enemies that may be closer than the swatting object
    float m_flStartExpireTimer;
    CountdownTimer m_EnemyScanTimer;
};
