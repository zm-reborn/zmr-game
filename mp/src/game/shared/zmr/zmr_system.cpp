#include "cbase.h"

#include "zmr/zmr_web.h"


class CZMSystem : public CAutoGameSystem
{
public:
    CZMSystem() : CAutoGameSystem( "ZMSystem" )
    {
    }

    virtual void PostInit() OVERRIDE;
};

void CZMSystem::PostInit()
{
    // Only init once when running the game, since this is called on both client and server.
#ifndef CLIENT_DLL
    if ( !engine->IsDedicatedServer() )
        return;
#endif


    g_pZMWeb->QueryVersionNumber();
}

CZMSystem g_ZMSystem;
