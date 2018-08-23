#include "cbase.h"
#include "GameEventListener.h"
#include <ctime>

#ifdef CLIENT_DLL
#include "physpropclientside.h"
#include "c_te_legacytempents.h"
#include "cdll_client_int.h"
#include "c_soundscape.h"

#include <engine/IEngineSound.h>


#include "zmr/c_zmr_zmvision.h"
#include "zmr/c_zmr_util.h"
#else
#include "zmr/zmr_rejoindata.h"
#endif

#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_web.h"


class CZMSystem : public CAutoGameSystem, public CGameEventListener
{
public:
    CZMSystem() : CAutoGameSystem( "ZMSystem" )
    {
    }

    virtual void PostInit() OVERRIDE;


    virtual void LevelShutdownPreEntity() OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;


#ifndef CLIENT_DLL
    void CheckSpecialDates();
#else
    void PrintRoundEndMessage( ZMRoundEndReason_t reason );
#endif


    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;
};

void CZMSystem::PostInit()
{
#ifdef CLIENT_DLL
    ListenForGameEvent( "round_end_post" );
    ListenForGameEvent( "round_restart_post" );
    ListenForGameEvent( "player_spawn" );
#endif

    // Server Steam API hasn't been initialized yet.
#ifdef CLIENT_DLL
    g_pZMWeb->QueryVersionNumber();
#endif
}

#ifndef CLIENT_DLL
ConVar zm_sv_happyzombies_usedate( "zm_sv_happyzombies_usedate", "1", FCVAR_NOTIFY, "Special dates bring happy zombies :)" );

void CZMSystem::CheckSpecialDates()
{
    if ( !zm_sv_happyzombies_usedate.GetBool() )
        return;


    time_t curtime;
    time( &curtime );
    tm* t = localtime( &curtime );

    HappyZombieEvent_t iEvent = HZEVENT_INVALID;

    // Christmas
    if ( t->tm_mon == 11 )
    {
        iEvent = HZEVENT_CHRISTMAS;
    }
    // Hulkamania
    else if ( t->tm_mon == 5 )
    {
        iEvent = HZEVENT_HULKAMANIA;
    }


    if ( iEvent != HZEVENT_INVALID )
    {
        ConVarRef( "zm_sv_happyzombies" ).SetValue( (int)iEvent );
        DevMsg( "Happy zombies activated by date!\n" );
    }
}
#endif

void CZMSystem::LevelShutdownPreEntity()
{
#ifndef CLIENT_DLL
    GetZMRejoinSystem()->OnLevelShutdown();
#endif
}

void CZMSystem::LevelInitPostEntity()
{
#ifndef CLIENT_DLL
    if ( engine->IsDedicatedServer() )
    {
        // HACK!!! After the first map change, the api hasn't been initialized yet.
        if ( steamgameserverapicontext && !steamgameserverapicontext->SteamHTTP() )
        {
            steamgameserverapicontext->Init();
        }

        g_pZMWeb->QueryVersionNumber();
    }

    CheckSpecialDates();
#else
    g_ZMVision.TurnOff();
#endif
}


void CZMSystem::FireGameEvent( IGameEvent* pEvent )
{
#ifdef CLIENT_DLL
    if ( Q_strcmp( pEvent->GetName(), "round_end_post" ) == 0 )
    {
        DevMsg( "Client received round end event!\n" );
        

        ZMRoundEndReason_t reason = (ZMRoundEndReason_t)pEvent->GetInt( "reason", ZMROUND_GAMEBEGIN );

        PrintRoundEndMessage( reason );
    }
    else if ( Q_strcmp( pEvent->GetName(), "round_restart_post" ) == 0 )
    {
        DevMsg( "Client received round restart event!\n" );


        // Read client-side phys props from map and recreate em.
        C_PhysPropClientside::RecreateAll();


        tempents->Clear();
        
        // Stop sounds.
        enginesound->StopAllSounds( true );
        Soundscape_OnStopAllSounds();


        // Clear decals.
        engine->ClientCmd( "r_cleardecals" );
        

        // Remove client ragdolls since they don't like getting removed.
        C_ClientRagdoll* pRagdoll;
        for ( C_BaseEntity* pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
        {
            pRagdoll = dynamic_cast<C_ClientRagdoll*>( pEnt );
            if ( pRagdoll )
            {
                // This will make them fade out.
                pRagdoll->SUB_Remove();
            }
        }
    }
    else if ( Q_strcmp( pEvent->GetName(), "player_spawn" ) == 0 )
    {
        // Tell the player they've spawned.
        C_ZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByUserId( pEvent->GetInt( "userid", -1 ) ) );
        if ( pPlayer )
        {
            pPlayer->OnSpawn();
        }
    }
#endif
}

#ifdef CLIENT_DLL
void CZMSystem::PrintRoundEndMessage( ZMRoundEndReason_t reason )
{
    const char* pMsg = nullptr;

    switch ( reason )
    {
    case ZMROUND_HUMANDEAD : pMsg = "#ZMRoundEndHumanDead"; break;
    case ZMROUND_HUMANLOSE : pMsg = "#ZMRoundEndHumanLose"; break;
    case ZMROUND_HUMANWIN : pMsg = "#ZMRoundEndHumanWin"; break;
    case ZMROUND_ZMSUBMIT : pMsg = "#ZMRoundEndSubmit"; break;
    case ZMROUND_GAMEBEGIN : pMsg = "#ZMRoundEndGameBegin"; break;
    case ZMROUND_VOTERESTART : pMsg = "#ZMRoundEndVoteRestart"; break;
    default : break;
    }

    if ( pMsg )
    {
        ZMClientUtil::PrintNotify( pMsg, ZMCHATNOTIFY_NORMAL );
    }
}
#endif

CZMSystem g_ZMSystem;
