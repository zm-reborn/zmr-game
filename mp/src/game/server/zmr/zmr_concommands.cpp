#include "cbase.h"

#include "items.h"


#include "zmr_ammo.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr_player.h"
#include "zmr/zmr_gamerules.h"
#include "zmr_voicelines.h"


/*
    Drop ammo
*/
void ZM_DropAmmo( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );
    
    if ( !pPlayer ) return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;



    CZMBaseWeapon* pWeapon = ToZMBaseWeapon( pPlayer->GetActiveWeapon() );

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

    src = pPlayer->EyePosition() + Vector( 0.0f, 0.0f, -12.0f );
    end = src + fwd * 38.0f;

    // Make sure we don't spawn in the wall.
    Vector testhull( 6, 6, 6 );
    
    UTIL_TraceHull( src, end, -testhull, testhull, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );


    CZMAmmo* ammobox = (CZMAmmo*)CBaseEntity::Create( ammoname, trace.endpos, pWeapon->GetAbsAngles(), nullptr );
    
    if ( !ammobox ) return;


    // Don't let players pickup this ammo instantly...
    ammobox->SetNextPickupTouch( 1.0f );


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


    CZMBaseWeapon* pWeapon = ToZMBaseWeapon( pPlayer->GetActiveWeapon() );

    if ( !pWeapon )
    {
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


void ZM_ObserveZombie( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsObserver() ) return;


    CZMBaseZombie* pZombie = nullptr;
    if ( args.ArgC() > 1 )
    {
        pZombie = ToZMBaseZombie( UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) ) );
    }
    else
    {
        // No argument, trace a line.
        CBaseEntity* pIgnore = (pPlayer->GetObserverTarget() && pPlayer->GetObserverMode() != OBS_MODE_ROAMING)
            ? pPlayer->GetObserverTarget()
            : pPlayer;

        trace_t tr;
        UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyeDirection3D() * MAX_COORD_FLOAT, MASK_NPCSOLID, pIgnore, COLLISION_GROUP_NPC, &tr );

        if ( tr.m_pEnt )
        {
            pZombie = ToZMBaseZombie( tr.m_pEnt );
        }
    }


    CBaseCombatCharacter* pCharacter = pZombie;

    // No valid zombie found. Try a random one.
    if ( !pCharacter || !pCharacter->IsAlive() )
    {
        pCharacter = nullptr;


        int i;
        CUtlVector<CBaseCombatCharacter*> vChars;
        CBaseEntity* pCurTarget = pPlayer->GetObserverTarget();
        
        // Flip from players to zombies and vice versa.
        if ( !pCurTarget || pCurTarget->IsPlayer() )
        {
            g_ZombieManager.ForEachAliveZombie( [ &vChars ]( CZMBaseZombie* pZombie )
            {
                vChars.AddToTail( pZombie );
            } );
        }
        else
        {
            for ( i = 1; i <= gpGlobals->maxClients; i++ )
            {
                CBasePlayer* pLoop = UTIL_PlayerByIndex( i );

                if ( pLoop && pLoop->IsAlive() )
                    vChars.AddToTail( pLoop );
            }
        }


        if ( vChars.Count() > 0 )
            pCharacter = vChars[random->RandomInt( 0, vChars.Count() - 1 )];
    }


    if ( pCharacter && pPlayer->SetObserverTarget( pCharacter ) )
    {
        pPlayer->SetObserverMode( OBS_MODE_CHASE );
    }
}

static ConCommand zm_observezombie( "zm_observezombie", ZM_ObserveZombie, "Allows observer to spectate a given zombie (ent index)." );


/*

*/
void ZM_VoiceMenu( const CCommand &args )
{
    if ( args.ArgC() <= 1 ) return;


    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );
    if ( !pPlayer )  return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;



    ZMGetVoiceLines()->OnVoiceLine( pPlayer, atoi( args.Arg( 1 ) ) );
}

static ConCommand zm_cmd_voicemenu( "zm_cmd_voicemenu", ZM_VoiceMenu );


/*
    Mirror legacy cvars to the new ones.
    Only set because some maps can break without these.
*/
#define LEGACY_CVAR( oldcvar, newcvar ) \
    CON_COMMAND( oldcvar, "DON'T EDIT THIS CVAR, USE "#newcvar" INSTEAD!" ) \
    { \
        extern ConVar newcvar; \
        if ( UTIL_IsCommandIssuedByServerAdmin() ) \
        { \
            Msg( "LEGACY CVAR %s | %s -> %s\n", #oldcvar, #newcvar, args.ArgS() ); \
            newcvar.SetValue( args.ArgS() ); \
        } \
    }


// taters_made_this
LEGACY_CVAR( zm_resource_limit, zm_sv_resource_max )
LEGACY_CVAR( zm_resource_refill_rate, zm_sv_resource_rate )
LEGACY_CVAR( zm_initial_resources, zm_sv_resource_init )

// bman
LEGACY_CVAR( zm_physexp_cost, zm_sv_physexp_cost )
LEGACY_CVAR( zm_spotcreate_cost, zm_hidden_cost_shambler )

// kink
LEGACY_CVAR( zm_flashlight_drainrate, zm_sv_flashlightdrainrate )
