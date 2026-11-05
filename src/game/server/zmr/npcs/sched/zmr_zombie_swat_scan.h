#pragma once

#include "npcs/zmr_zombiebase.h"

#include "zmr_zombie_main.h"
#include "zmr_zombie_swat_goto.h"



extern ConVar zm_sv_swat_scan_target_maxdist;
extern ConVar zm_sv_swat_scan_def_maxdist;


//
// Scan for a swattable object (can be breakable if parent wants to)
// and go swat it if found.
//
class CZombieScanSwatObjSchedule : public CBaseZombieSchedule, public CSwatInt
{
public:
    CZombieScanSwatObjSchedule();
    ~CZombieScanSwatObjSchedule();

    virtual const char* GetName() const override { return "ZombieScanSwatObj"; }


    virtual void OnStart() override;

    virtual void OnContinue() override;

    // We should never reach here.
    virtual void OnUpdate() override;

private:
    CBaseEntity* GetSwatObject();

    CBaseEntity* FindNearestSwatObject( const Vector& vecDirToGoal, const Vector& vecTarget, float flFurthestDist );


    CZombieGotoSwatObjSchedule* m_pGotoSwatSched;
};
