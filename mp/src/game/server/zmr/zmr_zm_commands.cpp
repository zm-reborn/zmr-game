#include "cbase.h"

//#include "hl2/npc_BaseZombie.h"

#include "func_break.h"


#include "npcs/zmr_zombiebase.h"
#include "zmr/zmr_global_shared.h"

#include "zmr_player.h"
#include "zmr_entities.h"


/*
    Move selected zombies.
*/
void ZM_Cmd_Move( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) );
    

    CZMBaseZombie* pZombie;

    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == pPlayer )
        {
            pZombie->Command( pos );
        }
    }
}

static ConCommand zm_cmd_move( "zm_cmd_move", ZM_Cmd_Move, "Move da zombies!", FCVAR_HIDDEN );


/*
    Swat objects / target players.
*/
void ZM_Cmd_Target( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;



    CBaseEntity* pTarget = UTIL_EntityByIndex( atof( args.Arg( 1 ) ) );

    bool bTarget = false;
    bool bSwat = false;
    bool bBreakable = false;

    if ( pTarget->IsPlayer() )
    {
        bTarget = true;
    }
    else
    {
        // If we're a physics object then swat.
        IPhysicsObject* phys = pTarget->VPhysicsGetObject();

        bSwat = phys && phys->IsMoveable();


        // If we're breakable (func_breakable, etc.) then try to break.
        CBreakable* pBreak = dynamic_cast<CBreakable*>( pTarget );

        bBreakable = pBreak && pBreak->GetHealth() > 0 && pBreak->IsBreakable() && pBreak->m_takedamage == DAMAGE_YES;
    }

    Msg( "Breakable: %i | Swat: %i | Target: %i\n", bBreakable, bSwat, bTarget );
    
    Vector pos;
    pos.x = atof( args.Arg( 2 ) );
    pos.y = atof( args.Arg( 3 ) );
    pos.z = atof( args.Arg( 4 ) );
    

    CZMBaseZombie* pZombie;
    
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( !pZombie ) continue;
        
        if ( pZombie->GetSelector() != pPlayer ) continue;


        if ( pZombie->CanSwatPhysicsObjects() && (bSwat || bBreakable) )
        {
            pZombie->Swat( pTarget );
        }
        else
        {
            if ( bTarget )
            {
                pZombie->TargetEnemy( pTarget );
            }
            else
            {
                pZombie->Command( pos );
            }
        }
    }
}

static ConCommand zm_cmd_target( "zm_cmd_target", ZM_Cmd_Target, "Selected zombies target given entity.", FCVAR_HIDDEN );


ConVar zm_sv_cost_hiddenshambler( "zm_sv_cost_hiddenshambler", "100", FCVAR_NOTIFY );

void ZM_Cmd_CreateHidden( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;
    

    if ( !CZMBaseZombie::HasEnoughPopToSpawn( ZMCLASS_SHAMBLER ) )
    {
        ClientPrint( pPlayer, HUD_PRINTCENTER, "You do not have enough population to do that!" );
        return;
    }
        

    if ( !pPlayer->HasEnoughRes( zm_sv_cost_hiddenshambler.GetInt() ) )
    {
        ClientPrint( pPlayer, HUD_PRINTCENTER, "You do not have enough resources to do that!" );
        return;
    }


    Vector pos;
    trace_t trace;

    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) ) + 1.0f;


    CZMEntTriggerBlockHidden* pBlock;
    for ( int i = 0; i < g_pBlockHidden->Count(); i++ )
    {
        pBlock = g_pBlockHidden->Element( i );

        if (pBlock && pBlock->IsActive()
        &&  pBlock->CollisionProp()
        &&  pBlock->CollisionProp()->IsPointInBounds( pos ) )
        {
            ClientPrint( pPlayer, HUD_PRINTTALK, "No hidden zombie may be created there!" );
            return;
        }
    }

    bool bVisible = false;


    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( !pPlayer->IsHuman() ) continue;

        if ( !pPlayer->IsAlive() ) continue;


        // ZMRTODO: Throw this somewhere else.
        UTIL_TraceLine( pPlayer->EyePosition(), pos, MASK_VISIBLE, pPlayer, COLLISION_GROUP_NONE, &trace );

        if ( trace.fraction == 1.0f )
        {
            bVisible = true;
            break;
        }

        
        UTIL_TraceLine( pPlayer->EyePosition(), pos + Vector( 0.0f, 0.0f, 64.0f ), MASK_VISIBLE, pPlayer, COLLISION_GROUP_NONE, &trace );

        if ( trace.fraction == 1.0f )
        {
            bVisible = true;
            break;
        }
    }

    if ( bVisible )
    {
        ClientPrint( pPlayer, HUD_PRINTCENTER, "A human can see that spot!" );
        return;
    }




    CZMBaseZombie* pZombie = dynamic_cast<CZMBaseZombie*>( CreateEntityByName( CZMBaseZombie::ClassToName( ZMCLASS_SHAMBLER ) ) );

    if ( !pZombie ) return;


    DispatchSpawn( pZombie );
    


    // Trace down to get ground position.
    UTIL_TraceHull(
        pos + Vector( 0.0f, 0.0f, 16.0f ),
        pos - Vector( 0.0f, 0.0f, 2.0f ),
        pZombie->GetHullMins(), pZombie->GetHullMaxs(),
        MASK_NPCSOLID, pZombie, COLLISION_GROUP_NONE, &trace );


    if (trace.startsolid || trace.fraction == 1.0f
    ||  (trace.fraction != 1.0f && trace.plane.normal[2] < 0.2)) // Is our spawn location ground?
    {
        ClientPrint( pPlayer, HUD_PRINTCENTER, "That is an invalid spot!" );

        pZombie->SUB_Remove();

        return;
    }

    // ZMRTODO: Face the zombie master.
    // Apparently just setting the angle here doesn't work...

    pZombie->Teleport( &trace.endpos, &vec3_angle, nullptr );

    pZombie->Activate();


    pPlayer->SetResources( pPlayer->GetResources() - zm_sv_cost_hiddenshambler.GetInt() );
}

static ConCommand zm_cmd_createhidden( "zm_cmd_createhidden", ZM_Cmd_CreateHidden, "Create da zombies!", FCVAR_HIDDEN );


void ZM_Cmd_Queue( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 3 ) return;


    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) ) );
    if ( !pSpawn ) return;


    ZombieClass_t zclass = static_cast<ZombieClass_t>( atoi( args.Arg( 2 ) ) );

    if ( !CZMBaseZombie::IsValidClass( zclass ) )
    {
        zclass = ZMCLASS_SHAMBLER;
    }


    int amount = atoi( args.Arg( 3 ) );
    if ( amount < 1 ) amount = 1;


    pSpawn->QueueUnit( pPlayer, zclass, amount );
}

static ConCommand zm_cmd_queue( "zm_cmd_queue", ZM_Cmd_Queue, "Usage: zm_cmd_queue <entindex> <number> | Create da zombies!", FCVAR_HIDDEN );


/*
    Clear zombie spawn queue
*/
void ZM_Cmd_QueueClear( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) ) );

    if ( !pSpawn ) return;


    int amount = atoi( args.Arg( 2 ) );

    int pos = atoi( args.Arg( 3 ) );
    if ( pos < 1 ) pos = -1; // Last?
    else --pos;

    pSpawn->QueueClear( amount, pos );
}

static ConCommand zm_cmd_queueclear( "zm_cmd_queueclear", ZM_Cmd_QueueClear, "Usage: zm_cmd_queueclear <entindex> <amount (def: 1)> <pos (def: last)>", FCVAR_HIDDEN );




void ZM_Cmd_SelectAll( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    int entindex = pPlayer->entindex();

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie )
        {
            pZombie->SetSelector( entindex );
        }
    }
}

static ConCommand zm_cmd_selectall( "zm_cmd_selectall", ZM_Cmd_SelectAll, "Select all da zombies!" );


void ZM_Cmd_UnSelectAll( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() )
    {
        return;
    }


    pPlayer->DeselectAllZombies();
}

static ConCommand zm_cmd_unselectall( "zm_cmd_unselectall", ZM_Cmd_UnSelectAll, "Unselect all da zombies!", FCVAR_HIDDEN );


void ZM_Cmd_Select( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;


    CBaseEntity* pEnt = UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) );

    if ( !pEnt ) return;


    // Testing
    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( pEnt );

    if ( pSpawn )
    {
        pSpawn->QueueUnit( pPlayer, ZMCLASS_SHAMBLER, 1 );
        return;
    }


    CZMBaseZombie* pZombie = dynamic_cast<CZMBaseZombie*>( pEnt );

    if ( pZombie )
    {
        if ( !atoi( args.Arg( 2 ) ) ) // Sticky?
        {
            pPlayer->DeselectAllZombies();
        }

        pZombie->SetSelector( pPlayer );
        return;
    }
}

static ConCommand zm_cmd_select( "zm_cmd_select", ZM_Cmd_Select, "Select spawns/traps/zombies, etc.", FCVAR_HIDDEN );



void ZM_Cmd_SelectMult( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 3 ) return;
    
    bool bSticky = atoi( args.Arg( 1 ) ) ? true : false;


    if ( !bSticky )
    {
        pPlayer->DeselectAllZombies();
    }


    CZMBaseZombie* pZombie;

    for ( int i = 2; i < args.ArgC(); i++ )
    {
        pZombie = dynamic_cast<CZMBaseZombie*>( UTIL_EntityByIndex( atoi( args.Arg( i ) ) ) );

        if ( pZombie )
        {
            pZombie->SetSelector( pPlayer );
        }
    }
}

static ConCommand zm_cmd_selectmult( "zm_cmd_selectmult", ZM_Cmd_SelectMult, "Select multiple zombies.", FCVAR_HIDDEN );


static ConVar zm_sv_gibdelete( "zm_sv_gibdelete", "1", FCVAR_NOTIFY, "Does deleting zombies gib them?" );

void ZM_Cmd_DeleteZombies( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;


    CTakeDamageInfo dmg;

    
    int dmgtype = zm_sv_gibdelete.GetBool() ? DMG_ALWAYSGIB : DMG_GENERIC;


    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == pPlayer )
        {
            dmg.SetDamageType( dmgtype );

            dmg.SetAttacker( pPlayer );
            dmg.SetInflictor( pZombie );

            dmg.SetDamage( pZombie->GetHealth() * 2 );
            dmg.SetDamagePosition( pZombie->GetAbsOrigin() );
            dmg.SetDamageForce( Vector( 0, 0, -10 ) );


            pZombie->TakeDamage( dmg );
        }
    }
}

static ConCommand zm_cmd_delete( "zm_cmd_delete", ZM_Cmd_DeleteZombies, "Delete selected zombies." );

/*
    Trigger a trap.
*/
void ZM_Cmd_Trigger( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;


    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap )
    {
        pTrap->Trigger( pPlayer );
    }
}

static ConCommand zm_cmd_trigger( "zm_cmd_trigger", ZM_Cmd_Trigger, "Trigger a trap.", FCVAR_HIDDEN );

/*
    Create trap trigger
*/
void ZM_Cmd_CreateTrigger( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 5 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 2 ) );
    pos.y = atof( args.Arg( 3 ) );
    pos.z = atof( args.Arg( 4 ) );
    

    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap )
    {
        if ( !pPlayer->HasEnoughRes( pTrap->GetTrapCost() ) )
        {
            ClientPrint( pPlayer, HUD_PRINTTALK, "You do not have enough resources for that!" );
            return;
        }


        pTrap->CreateTrigger( pos );

        
        pPlayer->SetResources( pPlayer->GetResources() - pTrap->GetTrapCost() );

        ClientPrint( pPlayer, HUD_PRINTTALK, "Created trap!" );
    }
}

static ConCommand zm_cmd_createtrigger( "zm_cmd_createtrigger", ZM_Cmd_CreateTrigger, "Create a trap trigger.", FCVAR_HIDDEN );


/*
    Create zombie spawn rally point
*/
void ZM_Cmd_SetRally( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 5 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 2 ) );
    pos.y = atof( args.Arg( 3 ) );
    pos.z = atof( args.Arg( 4 ) );
    

    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( entindex ) );

    if ( pSpawn )
    {
        pSpawn->SetRallyPoint( pos );

        ClientPrint( pPlayer, HUD_PRINTTALK, "Set zombie spawn rally point!" );
    }
}

static ConCommand zm_cmd_setrally( "zm_cmd_setrally", ZM_Cmd_SetRally, "Set rally point.", FCVAR_HIDDEN );


/*
    Open build menu
*/
void ZM_Cmd_OpenBuildMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;



    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( entindex ) );

    if ( pSpawn )
    {
        pPlayer->SetBuildSpawn( pSpawn );
        pSpawn->SendMenuUpdate();
    }
}

static ConCommand zm_cmd_openbuildmenu( "zm_cmd_openbuildmenu", ZM_Cmd_OpenBuildMenu, "", FCVAR_HIDDEN );


void ZM_Cmd_CloseBuildMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    pPlayer->SetBuildSpawn( 0 );
}

static ConCommand zm_cmd_closebuildmenu( "zm_cmd_closebuildmenu", ZM_Cmd_CloseBuildMenu, "", FCVAR_HIDDEN );



/*
    Open manipulate menu
*/
void ZM_Cmd_OpenManiMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;



    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap && pTrap->GetDescription()[0] )
    {
		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();


		UserMessageBegin( filter, "ZMManiMenuUpdate" );
			WRITE_STRING( pTrap->GetDescription() );
		MessageEnd();
    }
}

static ConCommand zm_cmd_openmanimenu( "zm_cmd_openmanimenu", ZM_Cmd_OpenManiMenu, "", FCVAR_HIDDEN );

