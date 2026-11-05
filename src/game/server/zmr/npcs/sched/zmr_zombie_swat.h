#pragma once

#include "npcs/zmr_zombiebase.h"
#include "zmr_zombie_main.h"


extern ConVar zm_sv_swatlift;
extern ConVar zm_sv_swatforcemin;
extern ConVar zm_sv_swatforcemax;
extern ConVar zm_sv_swatangvel;

//
// Interface for all swatting related schedules.
//
class CSwatInt
{
public:
    CSwatInt()
    {
        m_bBreakObject = true;
    }

    bool DoBreakObject() const { return m_bBreakObject; }
    void SetBreakObject( bool state ) { m_bBreakObject = state; }
private:
    bool m_bBreakObject;
};

//
// Zombie should be able to swat the object right now. Swat the object!
//
class CZombieSwatObjSchedule : public CBaseZombieSchedule, public CSwatInt
{
public:
    CZombieSwatObjSchedule();

    virtual const char* GetName() const override { return "ZombieSwatObj"; }

    virtual void OnStart() override;

    virtual void OnUpdate() override;

    virtual void OnAnimEvent( animevent_t* pEvent ) override;

    virtual NPCR::QueryResult_t IsBusy() const override;

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) override;

private:
    void DoSwatting();


    Vector m_vecFaceTowards;
    CountdownTimer m_FinishTimer;
    Activity m_SwatAct;
    bool m_bDidSwat;
    CHandle<CBaseEntity> m_hSwatObject;
};
