#include "cbase.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
#include "physpropclientside.h"
#include "c_te_legacytempents.h"
#include "cdll_client_int.h"
#include "c_soundscape.h"

#include <engine/IEngineSound.h>
#endif

#include "zmr/zmr_web.h"


class CZMSystem : public CAutoGameSystem, public CGameEventListener
{
public:
    CZMSystem() : CAutoGameSystem( "ZMSystem" )
    {
    }

    virtual void PostInit() OVERRIDE;
#ifndef CLIENT_DLL
    virtual void LevelInitPostEntity() OVERRIDE;
#endif


    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;
};

void CZMSystem::PostInit()
{
#ifdef CLIENT_DLL
    ListenForGameEvent( "round_restart_post" );
#endif

    // Server Steam API hasn't been initialized yet.
#ifdef CLIENT_DLL
    g_pZMWeb->QueryVersionNumber();
#endif
}

#ifndef CLIENT_DLL
void CZMSystem::LevelInitPostEntity()
{
    if ( engine->IsDedicatedServer() )
    {
        // HACK!!! After the first map change, the api hasn't been initialized yet.
        if ( steamgameserverapicontext && !steamgameserverapicontext->SteamHTTP() )
        {
            steamgameserverapicontext->Init();
        }

        g_pZMWeb->QueryVersionNumber();
    }
}
#endif

void CZMSystem::FireGameEvent( IGameEvent* pEvent )
{
#ifdef CLIENT_DLL
    if ( Q_strcmp( pEvent->GetName(), "round_restart_post" ) == 0 )
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
#endif
}

CZMSystem g_ZMSystem;
