#include "cbase.h"

#include "zmr_player.h"
#include "zmr_gamerules.h"


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

