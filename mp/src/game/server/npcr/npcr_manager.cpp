#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_manager.h"


NPCR::NPCManager NPCR::g_NPCManager;

static void OnUpdateConVarChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
    NPCR::g_NPCManager.UpdateSlots();
}

ConVar npcr_updaterate( "npcr_updaterate", "15", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many times a second we should update our NPCs", true, 1.0f, false, 0.0f, OnUpdateConVarChanged );
ConVar npcr_framelimit( "npcr_framelimit", "15", FCVAR_NOTIFY | FCVAR_ARCHIVE, "", true, 1.0f, false, 0.0f );
ConVar npcr_debug_noupdates( "npcr_debug_noupdates", "0" );
ConVar npcr_alwaysupdate( "npcr_alwaysupdate", "0" );
ConVar npcr_updatebalancing( "npcr_updatebalancing", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "Balance npc updates across all frames?", OnUpdateConVarChanged );
ConVar npcr_debug_manager( "npcr_debug_manager", "0" );
ConVar npcr_debug_manager_updatesum( "npcr_debug_manager_updatesum", "0" );


NPCR::NPCManager::NPCManager()
{
    m_iLastFrame = 0;
    m_flUpdateSum = 0.0f;
    m_flUpdateStartTime = 0.0f;
    m_iCurUpdateSlot = 0;
    m_nUpdateSlots = 0;
}

NPCR::NPCManager::~NPCManager()
{

}

bool NPCR::NPCManager::UsesUpdateSlots() const
{
    return m_nUpdateSlots > 1;
}

void NPCR::NPCManager::UpdateSlots()
{
    int newSlotCount = (int)floor( (1.0f / npcr_updaterate.GetInt()) / gpGlobals->interval_per_tick );
    
    // We don't want balancing
    if ( !npcr_updatebalancing.GetBool() )
        newSlotCount = 0;


    if ( newSlotCount == m_nUpdateSlots )
        return;


    m_nUpdateSlots = newSlotCount;
    m_iCurUpdateSlot = 0;

    // Reset the update slot for each npc
    if ( UsesUpdateSlots() )
    {
        FOR_EACH_VEC( m_vNPCs, i )
        {
            m_vNPCs[i]->m_iUpdateSlot = i % m_nUpdateSlots;
        }
    }
    else
    {
        m_nUpdateSlots = 0;

        FOR_EACH_VEC( m_vNPCs, i )
        {
            m_vNPCs[i]->m_iUpdateSlot = 0;
        }
    }


    Msg( "[NPCR] Updating npcs %i times a second with %i update slots @ %itick!\n",
        npcr_updaterate.GetInt(),
        m_nUpdateSlots,
        (int)(1.0f / gpGlobals->interval_per_tick) );
}

void NPCR::NPCManager::RegisterNPC( CBaseNPC* pNPC )
{
    Assert( m_vNPCs.Find( pNPC ) == m_vNPCs.InvalidIndex() );
    int index = m_vNPCs.AddToTail( pNPC );

    m_vNPCs[index]->m_iUpdateSlot = UsesUpdateSlots() ? (index % m_nUpdateSlots) : 0;

    UpdateSlots();
}

void NPCR::NPCManager::UnRegisterNPC( CBaseNPC* pNPC )
{
    Assert( m_vNPCs.Find( pNPC ) != m_vNPCs.InvalidIndex() );
    m_vNPCs.FindAndRemove( pNPC );
}

void NPCR::NPCManager::OnGameFrame()
{
    if ( m_iLastFrame == gpGlobals->framecount )
        return;
    m_iLastFrame = gpGlobals->framecount;



    if ( npcr_debug_manager_updatesum.GetBool() )
    {
        DevMsg( "[NPCR] Previous update: %.0f\n", m_flUpdateSum * 1000.0f );
    }


    m_flUpdateSum = 0.0f;
    
    int iDeltaTick = TIME_TO_TICKS( 1.0f / npcr_updaterate.GetFloat() );

    int iCurTick = gpGlobals->tickcount;
    FOR_EACH_VEC( m_vNPCs, i )
    {
        if ( (iCurTick - m_vNPCs[i]->m_iLastUpdateTick) < iDeltaTick )
            continue;

        if ( m_vNPCs[i]->m_iUpdateSlot != m_iCurUpdateSlot )
            continue;

        m_vNPCs[i]->m_bFlaggedForUpdate = true;
    }

    if ( UsesUpdateSlots() )
    {
        m_iCurUpdateSlot = (m_iCurUpdateSlot + 1) % m_nUpdateSlots;
    }
}

void NPCR::NPCManager::StartUpdate()
{
    m_flUpdateStartTime = engine->Time();
}

void NPCR::NPCManager::FinishUpdate()
{
    m_flUpdateSum += engine->Time() - m_flUpdateStartTime;
}

bool NPCR::NPCManager::ShouldUpdate( const NPCR::CBaseNPC* pNPC ) const
{
    if ( npcr_alwaysupdate.GetBool() )
        return true;

    if ( !pNPC->m_bFlaggedForUpdate )
        return false;

    if ( npcr_debug_noupdates.GetBool() )
        return false;


    float limit = npcr_framelimit.GetFloat();
    float sum = m_flUpdateSum * 1000.0f;

    if ( sum < limit )
    {
        return true;
    }


    if ( npcr_debug_manager.GetBool() )
    {
        Msg( "[NPCR] Exceeded frame limit! Current sum: %.1f | Limit: %.1f\n", sum, limit );
    }

    return false;
}

CON_COMMAND( npcr_remove_all, "" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }

    NPCR::g_NPCManager.ForEachNPC( []( NPCR::CBaseNPC* pNPC )
    {
        pNPC->RemoveNPC();
        return false;
    } );
}
