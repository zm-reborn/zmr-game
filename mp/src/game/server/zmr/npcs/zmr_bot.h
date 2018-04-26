#pragma once

#include "zmr/zmr_player.h"
#include "npcr/npcr_player.h"

class CZMPlayerBot : public NPCR::CPlayer<CZMPlayer>
{
public:
    CZMPlayerBot();
    ~CZMPlayerBot();


    static CZMPlayer* CreateZMBot( const char* playername = "" );


    virtual bool OverrideUserCmd( CUserCmd* pCmd ) OVERRIDE;

    // Called from NPCR::CPlayer to create the player entity
    static CBasePlayer* BotPutInServer( edict_t* pEdict, const char* playername );
};
