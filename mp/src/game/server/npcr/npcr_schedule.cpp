#include "cbase.h"
#include "vprof.h"

#include "npcr_basenpc.h"
#include "npcr_schedule.h"

namespace NPCR
{
    ConVar npcr_debug_schedules( "npcr_debug_schedules", "0" );
}


NPCR::CScheduleInterface::CScheduleInterface( NPCR::CBaseNPC* pNPC, NPCR::CScheduleInt* pInitialSched ) : NPCR::CEventDispatcher( pNPC, pNPC )
{
    Assert( pInitialSched );
    m_pInitialSched = pInitialSched;
    AddComponent( pInitialSched );
    m_pInitialSched->StartInitialSchedule();
}

NPCR::CScheduleInterface::~CScheduleInterface()
{
    delete m_pInitialSched;
}

void NPCR::CScheduleInterface::Update()
{
    VPROF_BUDGET( "CScheduleInterface::Update", "NPCR" );
    m_pInitialSched->Update();
}
