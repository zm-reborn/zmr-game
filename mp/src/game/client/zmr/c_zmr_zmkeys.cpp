#include "cbase.h"
#include "filesystem.h"
#include <inputsystem/iinputsystem.h>

#include "c_zmr_player.h"
#include "zmr_shareddefs.h"
#include "c_zmr_zmkeys.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// All commands we consider ZM commands
static const char* g_szZMCommands[] = {
    "+moveup",
    "+movedown",
    "+zm_cmd_ctrl",
    "zm_observermode",
    "zm_vision",
    "zm_cmd_selectall",
    "zm_cmd_delete",
    "zm_cmd_hiddenspawn",
    "roundrestart",
};

// All commands we consider survivor/spectator commands
static const char* g_szSurvivorCommands[] = {
    "+duck",
    "+jump",
    "+reload",
    "+speed",
    "+use",
    "+zm_voicemenu",
    "impulse 100",
    "impulse 201",
    "kill",
    "dropweapon",
    "dropammo",
    "zm_observezombie",
    "invnext",
    "invprev",
    "lastinv",
};


CZMTeamKeysConfig::CZMTeamKeysConfig()
{
}

CZMTeamKeysConfig::~CZMTeamKeysConfig()
{
}

bool CZMTeamKeysConfig::IsZMCommand( const char* cmd )
{
    for ( int i = 0; i < ARRAYSIZE( g_szZMCommands ); i++ )
    {
        if ( Q_stricmp( cmd, g_szZMCommands[i] ) == 0 )
            return true;
    }
   
    return false;
}

bool CZMTeamKeysConfig::IsSurvivorCommand( const char* cmd )
{
    for ( int i = 0; i < ARRAYSIZE( g_szSurvivorCommands ); i++ )
    {
        if ( Q_stricmp( cmd, g_szSurvivorCommands[i] ) == 0 )
            return true;
    }
   
    return false;
}

bool CZMTeamKeysConfig::IsNeutralCommand( const char* cmd )
{
    return !IsZMCommand( cmd ) && !IsSurvivorCommand( cmd );
}

void CZMTeamKeysConfig::ExecuteTeamConfig( bool bForce )
{
    auto* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( pPlayer )
    {
        ExecuteTeamConfig( pPlayer->IsZM() ? ZMTEAM_ZM : ZMTEAM_HUMAN );
    }
    else if ( bForce )
    {
        ExecuteTeamConfig( ZMTEAM_HUMAN );
    }
}

void CZMTeamKeysConfig::ExecuteTeamConfig( int iTeam )
{
    const char* cfg = iTeam != ZMTEAM_ZM ? KEYCONFIG_NAME_SURVIVOR : KEYCONFIG_NAME_ZM;
    const char* defcfg = iTeam != ZMTEAM_ZM ? KEYCONFIG_NAME_SURVIVOR"_default" : KEYCONFIG_NAME_ZM"_default";


    // Execute the default config if we can't find the player's own config.
    char cfgpath[128];
    Q_snprintf( cfgpath, sizeof( cfgpath ), "cfg/%s.cfg", cfg );

    if ( filesystem->FileExists( cfgpath ) )
    {
        engine->ClientCmd_Unrestricted( VarArgs( "exec %s.cfg", cfg ) );
    }
    else
    {
        engine->ClientCmd_Unrestricted( VarArgs( "exec %s.cfg", defcfg ) );
    }
}

ZMKeyTeam_t CZMTeamKeysConfig::GetCommandType( const char* cmd )
{
    if ( IsZMCommand( cmd ) )
    {
        return KEYTEAM_ZM;
    }
    if ( IsSurvivorCommand( cmd ) )
    {
        return KEYTEAM_SURVIVOR;
    }

    return KEYTEAM_NEUTRAL;
}

bool CZMTeamKeysConfig::LoadConfigByTeam( ZMKeyTeam_t team, zmkeydatalist_t& list )
{
    const char* cfg = team == KEYTEAM_SURVIVOR ? KEYCONFIG_SURVIVOR : KEYCONFIG_ZM;
    const char* defcfg = team == KEYTEAM_SURVIVOR ? KEYCONFIG_SURVIVOR_DEF : KEYCONFIG_ZM_DEF;


    bool res = CZMTeamKeysConfig::ParseConfig( cfg, list );

    if ( !res )
    {
        res = CZMTeamKeysConfig::ParseConfig( defcfg, list );
    }

    return res;
}

bool CZMTeamKeysConfig::ParseConfig( const char* cfg, zmkeydatalist_t& list )
{
    if ( !filesystem->FileExists( cfg ) )
    {
        return false;
    }


    auto hndl = filesystem->Open( cfg, "rb", "MOD" );
    if ( !hndl )
    {
        Warning( "Failed to open key config file '%s' for read!\n", cfg );
        return false;
    }


    char buf[512];
    buf[511] = NULL;

    char temp[32];
    char keyname[32];
    zmkeydata_t key;

    while ( filesystem->ReadLine( buf, sizeof( buf ) - 1, hndl ) != nullptr )
    {
        if ( buf[0] == NULL )
            continue;
        if ( buf[0] == '/' )
            continue;


        keyname[0] = NULL;
        key.cmd[0] = NULL;

        const char* data = buf;


        data = engine->ParseFile( data, temp, sizeof( temp ) );
        if ( Q_stricmp( temp, "bind" ) != 0 )
            continue;



        data = engine->ParseFile( data, keyname, sizeof( keyname ) );

        key.key = inputsystem->StringToButtonCode( keyname );

        if ( key.key <= KEY_NONE )
            continue;


        data = engine->ParseFile( data, key.cmd, sizeof( key.cmd ) );
        if ( key.cmd[0] == NULL )
            continue;


        list.AddToTail( new zmkeydata_t( key ) );
    }


    filesystem->Close( hndl );

    return true;
}

bool CZMTeamKeysConfig::SaveConfig( const char* cfg, const zmkeydatalist_t& list )
{
    auto hndl = filesystem->Open( cfg, "wb", "MOD" );
    if ( !hndl )
    {
        Warning( "Failed to open key config file '%s' for write!\n", cfg );
        return false;
    }


    char head[] =   "// These are the team specific keys.\n"
                    "// Please don't edit this file by hand unless you know what you're doing...\n";
                    
    filesystem->Write( head, sizeof( head ) - 1, hndl );

    char buf[512];
    
    
    FOR_EACH_VEC( list, i )
    {
        Q_snprintf( buf, sizeof( buf ), "bind \"%s\" \"%s\"\n",
            inputsystem->ButtonCodeToString( list[i]->key ),
            list[i]->cmd );
        filesystem->Write( buf, Q_strlen( buf ), hndl );
    }
    

    filesystem->Close( hndl );

    return true;
}

zmkeydata_t* CZMTeamKeysConfig::FindKeyDataFromList( const char* cmd, zmkeydatalist_t& list )
{
    FOR_EACH_VEC( list, i )
    {
        if ( Q_stricmp( cmd , list[i]->cmd ) == 0 )
        {
            return list[i];
        }
    }

    return nullptr;
}

zmkeydata_t* CZMTeamKeysConfig::FindKeyDataFromListByKey( ButtonCode_t key, zmkeydatalist_t& list )
{
    FOR_EACH_VEC( list, i )
    {
        if ( list[i]->key == key )
        {
            return list[i];
        }
    }

    return nullptr;
}
