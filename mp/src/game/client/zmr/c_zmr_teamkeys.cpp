#include "cbase.h"

#include "filesystem.h"
#include <inputsystem/iinputsystem.h>

#include "c_zmr_player.h"
#include "c_zmr_teamkeys.h"


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
    "zm_hiddenspawn",
    "roundrestart",
    "+zm_cmd_moveup",
    "+zm_cmd_movedown",
    "zm_cmd_select_radius",
};

// All commands we consider survivor/spectator commands
static const char* g_szSurvivorCommands[] = {
    "+duck",
    "+jump",
    "+reload",
    "+speed",
    "+use",
    "+attack3",
    "+zm_voicemenu",
    "impulse 100",
    "impulse 201",
    "kill",
    "dropweapon",
    "dropammo",
    "invnext",
    "invprev",
    "lastinv",
};

// Unique just for spectators.
static const char* g_szSpectatorCommands[] = {
    "zm_observezombie",
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

bool CZMTeamKeysConfig::IsSpectatorCommand( const char* cmd )
{
    for ( int i = 0; i < ARRAYSIZE( g_szSpectatorCommands ); i++ )
    {
        if ( Q_stricmp( cmd, g_szSpectatorCommands[i] ) == 0 )
            return true;
    }
   
    return false;
}

bool CZMTeamKeysConfig::IsNeutralCommand( const char* cmd )
{
    return !IsZMCommand( cmd ) && !IsSurvivorCommand( cmd ) && !IsSpectatorCommand( cmd );
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
    const char* cfg = TeamNumberToConfigName( iTeam, false );
    const char* defcfg = TeamNumberToConfigName( iTeam, true );


    // Execute the default config if we can't find the player's own config.
    char cfgpath[128];
    Q_snprintf( cfgpath, sizeof( cfgpath ), "cfg/%s.cfg", cfg );

    if ( filesystem->FileExists( cfgpath ) )
    {
		Q_snprintf( cfgpath, sizeof( cfgpath ), "exec %s.cfg", cfg );
        engine->ClientCmd_Unrestricted( cfgpath );
    }
    else
    {
		Q_snprintf( cfgpath, sizeof( cfgpath ), "exec %s.cfg", defcfg );
        engine->ClientCmd_Unrestricted( cfgpath );
    }
}

const char* CZMTeamKeysConfig::TeamNumberToConfigPath( int iTeam, bool bDefault )
{
	switch ( iTeam )
	{
	case 3 : // ZM
		return bDefault ? KEYCONFIG_ZM_DEF : KEYCONFIG_ZM;
	case 1 : // Spectator
	case 2 : // Survivor
	default :
		return bDefault ? KEYCONFIG_SURVIVOR_DEF : KEYCONFIG_SURVIVOR;
	}
}

const char* CZMTeamKeysConfig::TeamNumberToConfigName( int iTeam, bool bDefault )
{
	switch ( iTeam )
	{
	case 3 : // ZM
		return bDefault ? KEYCONFIG_NAME_ZM"_default" : KEYCONFIG_NAME_ZM;
	case 1 : // Spectator
        return bDefault ? KEYCONFIG_NAME_SPEC"_default" : KEYCONFIG_NAME_SPEC;
	case 2 : // Survivor
	default :
		return bDefault ? KEYCONFIG_NAME_SURVIVOR"_default" : KEYCONFIG_NAME_SURVIVOR;
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
	if ( IsSpectatorCommand( cmd ) )
	{
		return KEYTEAM_SPEC;
	}

    return KEYTEAM_NEUTRAL;
}

bool CZMTeamKeysConfig::IsKeyConflicted( ZMKeyTeam_t team1, ZMKeyTeam_t team2 )
{
    // We're in the same team, we conflict!
    if ( team1 == team2 )
        return true;


    // We're different teams.

    bool bHasNeutral = team1 == KEYTEAM_NEUTRAL || team2 == KEYTEAM_NEUTRAL;

    // Neutral command will always conflict with a team key.
    if ( bHasNeutral )
        return true;

    // ZM key does not conflict with survivor key, etc.
    return false;
}

bool CZMTeamKeysConfig::LoadConfigByTeam( ZMKeyTeam_t team, zmkeydatalist_t& list )
{
    const char* cfg = "";

    switch ( team )
    {
    case KEYTEAM_ZM :
        cfg = KEYCONFIG_ZM;
        break;
    case KEYTEAM_SURVIVOR :
        cfg = KEYCONFIG_SURVIVOR;
        break;
    case KEYTEAM_SPEC :
        cfg = KEYCONFIG_SPEC;
        break;
    default :
        Assert( 0 );
        break;
    }

    bool res = CZMTeamKeysConfig::ParseConfig( cfg, list );

    if ( !res )
    {
        res = LoadDefaultConfigByTeam( team, list );
    }

    return res;
}

bool CZMTeamKeysConfig::LoadDefaultConfigByTeam( ZMKeyTeam_t team, zmkeydatalist_t& list )
{
    const char* defcfg = "";

    switch ( team )
    {
    case KEYTEAM_ZM :
        defcfg = KEYCONFIG_ZM_DEF;
        break;
    case KEYTEAM_SURVIVOR :
        defcfg = KEYCONFIG_SURVIVOR_DEF;
        break;
    case KEYTEAM_SPEC :
        defcfg = KEYCONFIG_SPEC_DEF;
        break;
    default :
        Assert( 0 );
        break;
    }

    return CZMTeamKeysConfig::ParseConfig( defcfg, list );
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
        Assert( list[i]->key > KEY_NONE );

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
