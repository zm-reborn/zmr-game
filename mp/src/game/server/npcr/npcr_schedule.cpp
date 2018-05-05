#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_schedule.h"


ConVar npcr_debug_schedules( "npcr_debug_schedules", "0" );


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
    m_pInitialSched->Update();
}
