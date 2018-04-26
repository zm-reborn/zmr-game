#include "cbase.h"
#include "gameinterface.h"

#include "zmr_bot.h"



extern ConVar bot_mimic;
extern ConVar bot_mimic_pitch_offset;
extern ConVar bot_mimic_yaw_offset;


LINK_ENTITY_TO_CLASS( npcr_player_zm, CZMPlayerBot );
PRECACHE_REGISTER( npcr_player_zm );


CZMPlayerBot::CZMPlayerBot()
{
}

CZMPlayerBot::~CZMPlayerBot()
{
}

CZMPlayer* CZMPlayerBot::CreateZMBot( const char* playername )
{
    char name[128];
    Q_strncpy( name, ( playername && (*playername) ) ? playername : "L Ron Hubbard", sizeof( name ) );


    CZMPlayer* pPlayer = NPCR::CPlayer<CZMPlayer>::CreateBot<CZMPlayerBot>( name );

    return pPlayer;
}

CBasePlayer* CZMPlayerBot::BotPutInServer( edict_t* pEdict, const char* playername )
{
    CZMPlayer* pPlayer = CreatePlayer( "npcr_player_zm", pEdict );

    if ( pPlayer )
    {
        pPlayer->SetPlayerName( playername );
        pPlayer->SetPickPriority( 0 );

        pPlayer->ClearFlags();
        pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
    }

    return pPlayer;
}

bool CZMPlayerBot::OverrideUserCmd( CUserCmd* pCmd )
{
    if ( bot_mimic.GetInt() <= 0 )
        return false;

    CBasePlayer* pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt() );
    if ( !pPlayer )
        return false;

    const CUserCmd* pPlyCmd = pPlayer->GetLastUserCommand();
    if ( !pPlyCmd )
        return false;


    *pCmd = *pPlyCmd;
    pCmd->viewangles[PITCH] += bot_mimic_pitch_offset.GetFloat();
    pCmd->viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

    return true;
}

CON_COMMAND( bot, "" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }


    CZMPlayerBot::CreateZMBot();
}

CON_COMMAND( zm_bot, "" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }


    const char* name = args.Arg( 1 );

    CZMPlayerBot::CreateZMBot( name );
}
