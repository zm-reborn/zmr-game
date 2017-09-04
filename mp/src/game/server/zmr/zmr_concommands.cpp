#include "cbase.h"

#include "items.h"


#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr_player.h"
#include "zmr/zmr_gamerules.h"


/*
    Drop ammo
*/
void ZM_DropAmmo( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );
    
    if ( !pPlayer ) return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;



    CZMBaseWeapon* pWeapon = dynamic_cast<CZMBaseWeapon*>( pPlayer->GetActiveWeapon() );

    if ( !pWeapon ) return;

    if ( !pWeapon->CanBeDropped() ) return;


    const char* ammoname = pWeapon->GetDropAmmoName();
    int amount = pWeapon->GetDropAmmoAmount();
    
    // We have no ammo to drop.
    if ( !ammoname ) return;

    if ( pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) < amount )
    {
        return;
    }

    trace_t trace;
    Vector src, end;
    Vector fwd;

    pPlayer->EyeVectors( &fwd, nullptr, nullptr );

    src = pPlayer->EyePosition();
    end = pPlayer->EyePosition() + fwd * 70.0f;

    // Make sure we don't spawn in the wall.
    Vector testhull( 6, 6, 6 );
    
    UTIL_TraceHull( src, end, -testhull, testhull, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );


    CItem* ammobox = (CItem*)CBaseEntity::Create( ammoname, trace.endpos, pWeapon->GetAbsAngles(), nullptr );
    
    if ( !ammobox ) return;


    IPhysicsObject* pPhys = ammobox->VPhysicsGetObject();

    if ( pPhys )
    {
        Vector vel = pPlayer->GetAbsVelocity() + fwd * 200.0f;

        AngularImpulse angvel( 100, 100, 100 );

        pPhys->AddVelocity( &vel, &angvel );
    }

    ammobox->AddSpawnFlags( SF_NORESPAWN );

    pPlayer->RemoveAmmo( amount, pWeapon->GetPrimaryAmmoType() );
}

static ConCommand dropammo( "dropammo", ZM_DropAmmo, "Drop your ammo from your current weapon." );

/*
    Drop weapon
*/
void ZM_DropWeapon( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );
    
    if ( !pPlayer ) return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;


    CZMBaseWeapon* pWeapon = dynamic_cast<CZMBaseWeapon*>( pPlayer->GetActiveWeapon() );

    if ( !pWeapon )
    {
        // If we're a stock weapon then just remove us.
        CBaseCombatWeapon* pBase = pPlayer->GetActiveWeapon();

        if ( pBase )
        {
            pPlayer->Weapon_Drop( pBase );
            UTIL_Remove( pBase );
        }

        return;
    }

    if ( !pWeapon->CanBeDropped() ) return;


    pPlayer->Weapon_Drop( pWeapon, nullptr, nullptr );
}

static ConCommand dropweapon( "dropweapon", ZM_DropWeapon, "Drop your current weapon" );

/*
    Round restart
*/
void ZM_RoundRestart( const CCommand &args )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

        if ( !pPlayer ) return;

        if ( !pPlayer->IsZM() && !sv_cheats->GetBool() )
        {
            // ZMRTODO: Vote for round restart.
            return;
        }
    }
    

    ZMRules()->EndRound( ZMROUND_ZMSUBMIT );
}

static ConCommand roundrestart( "roundrestart", ZM_RoundRestart, "Restart the round as ZM." );


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
