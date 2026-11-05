#pragma once

#include "npcs/zmr_playerbot.h"

#include "zmr_bot_main.h"

#include "npcr_schedule.h"
#include "npcr_path_chase.h"


//
// Follow real players.
//
class CSurvivorFollowSchedule : public CBasePlayerBotSchedule
{
public:
    CSurvivorFollowSchedule();
    ~CSurvivorFollowSchedule();

    virtual const char* GetName() const override { return "SurvivorFollow"; }

    virtual void OnStart() override;

    virtual void OnContinue() override;

    virtual void OnUpdate() override;

    virtual void OnSpawn() override;

    virtual void OnHeardSound( CSound* pSound ) override;

    //virtual NPCR::QueryResult_t IsBusy() const override;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;

    //virtual void OnMoveSuccess( NPCR::CBaseNavPath* pPath ) override;


private:
    bool IsValidFollowTarget( CBasePlayer* pPlayer, bool bCheckLoop = false ) const;

    void NextFollow();

    void StartFollow( CBasePlayer* pFollow );

    bool ShouldMoveCloser( CBasePlayer* pFollow ) const;

    CBasePlayer* FindSurvivorToFollow( CBasePlayer* pIgnore = nullptr, bool bAllowBot = false ) const;


    NPCR::CChaseNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    CountdownTimer m_NextFollowTarget;

    CHandle<CBasePlayer> m_hFollowTarget;
};
