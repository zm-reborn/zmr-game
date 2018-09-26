#include "cbase.h"
#include "basecombatcharacter.h"
#include "vprof.h"

#include "npcr_schedule.h"
#include "npcr_motor.h"
#include "npcr_basenpc.h"
#include "npcr_senses.h"
#include "npcr_manager.h"


ConVar npcr_debug_componentcreation( "npcr_debug_componentcreation", "0" );
ConVar npcr_removeonfailure( "npcr_removeonfailure", "1" );

extern ConVar npcr_updaterate;


NPCR::CBaseNPC::CBaseNPC( CBaseCombatCharacter* pChar ) : NPCR::CNPCInterface( pChar ), NPCR::CEventDispatcher( nullptr, this )
{
    m_flLastUpdate = 0.0f;
    m_flUpdateTime = 0.1f;
    m_bFlaggedForUpdate = false;
    m_iLastUpdateTick = 0;
    m_iUpdateSlot = 0;


    g_NPCManager.RegisterNPC( this );
}

NPCR::CBaseNPC::~CBaseNPC()
{
    g_NPCManager.UnRegisterNPC( this );
}

NPCR::CBaseComponents::~CBaseComponents()
{
    delete m_pSchedInterface;
    delete m_pMotor;
    delete m_pSenses;
}

void NPCR::CBaseNPC::PostConstructor()
{
    if ( CreateComponents() )
    {
        OnCreatedComponents();
    }
    else
    {
        Warning( "Couldn't create all NPC components!!\n" );

        if ( npcr_removeonfailure.GetBool() )
        {
            RemoveNPC();
            return;
        }
    }
}

bool NPCR::CBaseComponents::CreateComponents()
{
    m_pSchedInterface = CreateScheduleInterface();
    if ( !m_pSchedInterface )
    {
        Warning( "No schedule interface created!\n" );
        return false;
    }

    m_pMotor = CreateMotor();
    if ( !m_pMotor )
    {
        Warning( "No motor created!\n" );
        return false;
    }

    m_pSenses = CreateSenses();
    if ( !m_pSenses )
    {
        Warning( "No senses created!\n" );
        return false;
    }

    return true;
}

NPCR::CScheduleInterface* NPCR::CBaseComponents::GetScheduleInterface() const
{
    return m_pSchedInterface;
}

NPCR::CBaseMotor* NPCR::CBaseComponents::GetMotor() const
{
    return m_pMotor;
}

NPCR::CBaseSenses* NPCR::CBaseComponents::GetSenses() const
{
    return m_pSenses;
}

void NPCR::CBaseNPC::OnCreatedComponents()
{
    if ( npcr_debug_componentcreation.GetBool() )
    {
        int c = m_Components.Count();
        Msg( "Created %i components:\n", c );
        for ( int i = 0; i < c; i++ )
        {
            Msg( "Component: %s\n", m_Components[i]->GetComponentName() );
        }
    }
}

bool NPCR::CBaseNPC::ShouldUpdate() const
{
    return g_NPCManager.ShouldUpdate( this );
}

void NPCR::CBaseNPC::Update()
{
    VPROF_BUDGET( "CBaseNPC::Update", "NPCR" );

    if ( ShouldUpdate() )
    {
        g_NPCManager.StartUpdate();

        UpdateInterval();

        PreUpdate();
        CEventDispatcher::Update();
        PostUpdate();

        FinishUpdate();
    }
}

void NPCR::CBaseNPC::FinishUpdate()
{
    m_bFlaggedForUpdate = false;

    m_iLastUpdateTick = gpGlobals->tickcount;
    m_flLastUpdate = gpGlobals->curtime;

    g_NPCManager.FinishUpdate();
}

void NPCR::CBaseNPC::UpdateInterval()
{
    // We haven't updated yet.
    if ( m_flLastUpdate == 0.0f )
    {
        // Just assume the update rate.
        m_flLastUpdate = gpGlobals->curtime - (1.0f / npcr_updaterate.GetFloat());
    }


    m_flUpdateTime = gpGlobals->curtime - m_flLastUpdate;
}
