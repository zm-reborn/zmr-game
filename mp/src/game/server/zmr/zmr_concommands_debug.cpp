#include "cbase.h"

#include "zmr_player.h"
#include "zmr_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


/*
    Endround (debugging)
*/
void ZM_EndRound( const CCommand &args )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

        if ( !pPlayer ) return;

        if ( !pPlayer->IsZM() && !sv_cheats->GetBool() ) return;
    }


    ZMRoundEndReason_t reason = ZMROUND_ZMSUBMIT;


    if ( args.ArgC() > 1 )
        reason = (ZMRoundEndReason_t)atoi( args.Arg( 1 ) );


    ZMRules()->EndRound( reason );
}

static ConCommand zm_endround( "zm_endround", ZM_EndRound, "Usage: zm_endround <reason number>" );


/*
    Forceteam (debugging)
*/
void ZM_ForceTeam( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );


    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        if ( !pPlayer ) return;

        if ( !sv_cheats->GetBool() ) return;
    }
    
    
    if ( args.ArgC() < 2 ) return;


    int iTeam = atoi( args.Arg( 1 ) );

    CZMPlayer* pTarget = nullptr;

    if ( args.ArgC() < 3 )
    {
        pTarget = pPlayer;
    }
    else
    {
        const char* name = args.Arg( 2 );
        int len = strlen( name );

        CZMPlayer* pPlayer;
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

            if ( pPlayer && Q_strnicmp( name, pPlayer->GetPlayerName(), len ) == 0 )
            {
                pTarget = pPlayer;
                break;
            }
        }
    }


    if ( iTeam <= ZMTEAM_UNASSIGNED ) return;

    if ( !pTarget ) return;


    pTarget->ChangeTeam( iTeam );
}

static ConCommand zm_forceteam( "zm_forceteam", ZM_ForceTeam, "Usage: zm_forceteam <number> <name (optional)> | 1 = Spec, 2 = Human, 3 = ZM" );


/*
    Force human
*/
void ZM_ForceHuman( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;


    pPlayer->ChangeTeam( ZMTEAM_HUMAN );
}

static ConCommand forcehuman( "forcehuman", ZM_ForceHuman, "", FCVAR_CHEAT );


/*
    Force zm
*/
void ZM_ForceZM( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;


    pPlayer->ChangeTeam( ZMTEAM_ZM );
}

static ConCommand forcezm( "forcezm", ZM_ForceZM, "", FCVAR_CHEAT );


/*
    Force spectator
*/
void ZM_ForceSpec( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;


    pPlayer->ChangeTeam( ZMTEAM_SPECTATOR );
}

static ConCommand forcespec( "forcespec", ZM_ForceSpec, "", FCVAR_CHEAT );


/*
    Set health (debugging)
*/
void ZM_SetHealth( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        if ( !pPlayer ) return;

        if ( !sv_cheats->GetBool() ) return;
    }
    

    if ( args.ArgC() < 2 ) return;


    int health = atoi( args.Arg( 1 ) );

    CZMPlayer* pTarget = nullptr;

    if ( args.ArgC() < 3 )
    {
        pTarget = pPlayer;
    }
    else
    {
        const char* name = args.Arg( 2 );
        int len = strlen( name );

        CZMPlayer* pPlayer;
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

            if ( pPlayer && Q_strnicmp( name, pPlayer->GetPlayerName(), len ) == 0 )
            {
                pTarget = pPlayer;
                break;
            }
        }
    }


    if ( !pTarget ) return;


    pTarget->SetHealth( health );
}

static ConCommand zm_sethealth( "zm_sethealth", ZM_SetHealth, "Usage: zm_sethealth <number> <name (optional)>" );

/*
    Set health (debugging)
*/
void ZM_GiveResources( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        if ( !pPlayer ) return;

        if ( !sv_cheats->GetBool() ) return;
    }
    

    if ( args.ArgC() < 2 ) return;


    int res = atoi( args.Arg( 1 ) );

    CZMPlayer* pTarget = nullptr;

    if ( args.ArgC() < 3 )
    {
        pTarget = pPlayer;
    }
    else
    {
        const char* name = args.Arg( 2 );
        int len = strlen( name );

        CZMPlayer* pPlayer;
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

            if ( pPlayer && Q_strnicmp( name, pPlayer->GetPlayerName(), len ) == 0 )
            {
                pTarget = pPlayer;
                break;
            }
        }
    }


    if ( !pTarget ) return;


    pTarget->IncResources( res );
}

static ConCommand zm_giveresources( "zm_giveresources", ZM_GiveResources, "Usage: zm_giveresources <number> <name (optional)>" );


/*
    Print pick priority (debugging)
*/
void ZM_PrintPriority( const CCommand &args )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }
    

    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( pPlayer )
        {
            Msg( "%s: %i\n", pPlayer->GetPlayerName(), pPlayer->GetPickPriority() );
        }
    }

}

static ConCommand zm_printpriority( "zm_printpriority", ZM_PrintPriority );

/*
    Gib all alive zombies.
*/
void ZM_GibZombies( const CCommand &args )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }
    

    g_ZombieManager.ForEachAliveZombie( []( CZMBaseZombie* pZombie )
    {
        CTakeDamageInfo info( pZombie, pZombie, 1337.0f, DMG_ALWAYSGIB );
        pZombie->TakeDamage( info );
    } );
}

static ConCommand zm_gibzombies( "zm_gibzombies", ZM_GibZombies );

/*
    Create zombie at crosshair.
*/
static int zm_zombie_create_completion( const char* partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
    // Autocomplete zombie classnames.
    const char* completions[] = {
        "npc_zombie",
        "npc_fastzombie",
        "npc_poisonzombie",
        "npc_dragzombie",
        "npc_burnzombie"
    };


    char cmd[128];
    Q_strncpy( cmd, partial, sizeof( cmd ) );

    
    // Skip the command part to compare the argument.
    auto* pszArg = Q_strstr( cmd, " " );
    if ( pszArg )
    {
        *pszArg = NULL;

        ++pszArg;

        if ( !Q_strlen( pszArg ) )
            pszArg = nullptr;
    }


    int cmds = 0;
    for ( int i = 0; i < ARRAYSIZE( completions ); i++ )
    {
        if ( !pszArg || Q_strstr( completions[i], pszArg ) == completions[i] )
        {
            Q_snprintf( commands[cmds++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", cmd, completions[i] );
        }
    }

    return cmds;
}

static void ZM_Zombie_Create( const CCommand& args )
{
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if ( !pPlayer ) return;

    if ( !UTIL_IsCommandIssuedByServerAdmin() && !sv_cheats->GetBool() )
    {
        return;
    }


    Vector fwd;
    trace_t tr;
    AngleVectors( pPlayer->EyeAngles(), &fwd );
    UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + fwd * MAX_COORD_FLOAT, MASK_NPCSOLID & ~CONTENTS_MONSTER, pPlayer, COLLISION_GROUP_NONE, &tr );

    if ( tr.fraction == 1.0f || tr.startsolid )
        return;


    const char* classname = "npc_zombie";
    const char* arg = args.Arg( 1 );
    if ( arg && *arg )
    {
        classname = arg;
    }


    CBaseEntity* pEnt = CreateEntityByName( classname );
    if ( !pEnt )
        return;

    CZMBaseZombie* pZombie = dynamic_cast<CZMBaseZombie*>( pEnt );
    if ( !pZombie || !pZombie->CanSpawn( tr.endpos ) )
    {
        UTIL_RemoveImmediate( pEnt );
        return;
    }

    pZombie->SetAbsOrigin( tr.endpos );


    QAngle ang = pPlayer->EyeAngles();
    ang.x = ang.z = 0.0f;
    pZombie->SetAbsAngles( ang );

    DispatchSpawn( pZombie );
}

#define ZOMBIECREATE_DESC "Creates a zombie at your crosshair. Takes a zombie classname."

static ConCommand zm_zombie_create( "zm_zombie_create", ZM_Zombie_Create, ZOMBIECREATE_DESC, 0, zm_zombie_create_completion );
static ConCommand npc_create( "npc_create", ZM_Zombie_Create, ZOMBIECREATE_DESC, 0, zm_zombie_create_completion );
