#include "cbase.h"

#include "items.h"


#include "zmr_ammo.h"
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


    CZMBaseWeapon* pWeapon = dynamic_cast<CZMBaseWeapon*>( pPlayer->GetActiveWeapon() );

    if ( !pWeapon )
    {
        // If we're a stock weapon then just remove us.
        CBaseCombatWeapon* pBase = pPlayer->GetActiveWeapon();

        if ( pBase )
        {
            pPlayer->Weapon_Drop( pBase, nullptr, nullptr );
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
