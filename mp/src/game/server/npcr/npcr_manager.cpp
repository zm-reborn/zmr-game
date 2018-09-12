#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_manager.h"


NPCR::NPCManager NPCR::g_NPCManager;

ConVar npcr_updaterate( "npcr_updaterate", "15", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many times a second we should update our NPCs", true, 1.0f, false, 0.0f );
ConVar npcr_framelimit( "npcr_framelimit", "15", FCVAR_NOTIFY | FCVAR_ARCHIVE, "", true, 1.0f, false, 0.0f );
ConVar npcr_debug_noupdates( "npcr_debug_noupdates", "0" );
ConVar npcr_alwaysupdate( "npcr_alwaysupdate", "0" );
ConVar npcr_debug_manager( "npcr_debug_manager", "0" );


NPCR::NPCManager::NPCManager()
{
    m_iLastFrame = 0;
    m_flUpdateSum = 0.0f;
    m_flUpdateStartTime = 0.0f;
}

NPCR::NPCManager::~NPCManager()
{

}

void NPCR::NPCManager::RegisterNPC( CBaseNPC* pNPC )
{
    Assert( m_vNPCs.Find( pNPC ) == m_vNPCs.InvalidIndex() );
    m_vNPCs.AddToTail( pNPC );
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


    m_flUpdateSum = 0.0f;
    
    int iDeltaTick = TIME_TO_TICKS( 1.0f / npcr_updaterate.GetFloat() );

    int iCurTick = gpGlobals->tickcount;
    FOR_EACH_VEC( m_vNPCs, i )
    {
        if ( (iCurTick - m_vNPCs[i]->m_iLastUpdateTick) < iDeltaTick )
            continue;

        m_vNPCs[i]->m_bFlaggedForUpdate = true;
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
