#include "cbase.h"

//#include "hl2/npc_BaseZombie.h"

#include "func_break.h"
#include "IEffects.h"
#include "envspark.h"


#include "npcs/zmr_zombiebase_shared.h"
#include "npcs/zmr_banshee.h"
//#include "npcs/zmr_fastzombie.h"
#include "zmr_util.h"

#include "zmr_player.h"
#include "zmr_entities.h"
#include "zmr_hiddenspawn.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


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

    ZMUtil::MoveSelectedZombies( pPlayer->entindex(), pos );
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



    CBaseEntity* pTarget = UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) );


    if ( !pTarget ) return;


    // ZM wants to break it.
    bool bForceBreak = atoi( args.Arg( 2 ) ) ? true : false;


    bool bTarget = false; // Just target an enemy.
    bool bSwat = false; // Just swat the object away/towards an enemy.
    bool bIsBreakable = false;
    bool bCanBeDamaged = pTarget->GetHealth() > 0 && pTarget->m_takedamage == DAMAGE_YES;
    bool bIsOnZombieTeam = pTarget->GetTeamNumber() == ZMTEAM_ZM; // Don't attack objects associated with the ZM.
    
    if ( pTarget->IsPlayer() )
    {
        bTarget = true;
    }
    else
    {
        bSwat = CZMBaseZombie::CanSwatObject( pTarget );

        if ( bCanBeDamaged )
        {
            CBreakable* pBreak = dynamic_cast<CBreakable*>( pTarget );
            bIsBreakable = pBreak && pBreak->IsBreakable();
        }
    }


    Vector pos;
    pos.x = atof( args.Arg( 3 ) );
    pos.y = atof( args.Arg( 4 ) );
    pos.z = atof( args.Arg( 5 ) );


    // Can't really do anything here, just move.
    if ( (!bTarget && !bCanBeDamaged && !bSwat) || bIsOnZombieTeam )
    {
        ZMUtil::MoveSelectedZombies( pPlayer->entindex(), pos );
        return;
    }
    


    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ bSwat, bCanBeDamaged, bIsBreakable, bForceBreak, bTarget, pTarget, &pos, pPlayer ]( CZMBaseZombie* pZombie )
    {
        bool bCanSwat = bSwat && pZombie->CanSwatPhysicsObjects();
        bool bCanBreak = bCanBeDamaged && ( pZombie->CanSwatPhysicsObjects() || bIsBreakable );

        if ( bCanSwat || bCanBreak )
        {
            // If we can swat, we need to be forced to break.
            // If we can't swat, we'll break it!
            pZombie->Swat( pPlayer, pTarget, (bCanSwat && bForceBreak && bCanBreak) || !bCanSwat );
        }
        else
        {
            if ( bTarget )
            {
                //pZombie->TargetEnemy( pTarget );
            }
            else
            {
                pZombie->Command( pPlayer, pos );
            }
        }
    } );
}

static ConCommand zm_cmd_target( "zm_cmd_target", ZM_Cmd_Target, "Selected zombies target given entity.", FCVAR_HIDDEN );


ConVar zm_sv_cost_hiddenshambler( "zm_sv_cost_hiddenshambler", "100", FCVAR_NOTIFY );

void ZM_Cmd_CreateHidden( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !UTIL_IsCommandIssuedByServerAdmin() && !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;


    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) ) + 1.0f;


    ZombieClass_t zclass = ZMCLASS_SHAMBLER;

    
    g_ZMHiddenSpawn.Spawn( zclass, pPlayer, pos );
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

    if ( g_ZombieManager.GetNumZombies() < 1 ) return;


    int entindex = pPlayer->entindex();

    g_ZombieManager.ForEachAliveZombie( [ entindex ]( CZMBaseZombie* pZombie )
    {
        pZombie->SetSelector( entindex );
    } );

    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMSelectAll" );
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


    CZMBaseZombie* pZombie = ToZMBaseZombie( pEnt );

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
        pZombie = ToZMBaseZombie( UTIL_EntityByIndex( atoi( args.Arg( i ) ) ) );

        if ( pZombie )
        {
            pZombie->SetSelector( pPlayer );
        }
    }
}

static ConCommand zm_cmd_selectmult( "zm_cmd_selectmult", ZM_Cmd_SelectMult, "Select multiple zombies.", FCVAR_HIDDEN );


ConVar zm_sv_gibdelete( "zm_sv_gibdelete", "1", FCVAR_NOTIFY, "Does deleting zombies gib them?" );

void ZM_Cmd_DeleteZombies( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;


    CTakeDamageInfo dmg;

    
    int dmgtype = zm_sv_gibdelete.GetBool() ? DMG_ALWAYSGIB : DMG_GENERIC;


    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ &dmg, dmgtype, pPlayer ]( CZMBaseZombie* pZombie )
    {
        dmg.SetDamageType( dmgtype );

        dmg.SetAttacker( pPlayer );
        dmg.SetInflictor( pZombie );

        dmg.SetDamage( pZombie->GetHealth() * 2 );
        dmg.SetDamagePosition( pZombie->GetAbsOrigin() );
        dmg.SetDamageForce( Vector( 0, 0, -10 ) );


        pZombie->TakeDamage( dmg );
    } );
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
ConVar zm_sv_maxtraptriggers( "zm_sv_maxtraptriggers", "3", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many triggers can the ZM create for one trap?" );

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
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
            return;
        }

        if ( pTrap->GetTriggerCount() >= zm_sv_maxtraptriggers.GetInt() )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMMaxTrapTriggers" );
            return;
        }


        pTrap->CreateTrigger( pos );

        
        pPlayer->IncResources( -pTrap->GetTrapCost() );

        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedTrap" );
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

        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedRally" );
    }
}

static ConCommand zm_cmd_setrally( "zm_cmd_setrally", ZM_Cmd_SetRally, "Set rally point.", FCVAR_HIDDEN );


/*
    Open build menu
*/
void ZM_Cmd_OpenMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;



    CZMEntBaseUsable* pEntity = dynamic_cast<CZMEntBaseUsable*>( UTIL_EntityByIndex( entindex ) );

    if ( pEntity )
    {
        pPlayer->SetMenuEnt( pEntity );
        pEntity->SendMenuUpdate();
    }
}

static ConCommand zm_cmd_openmenu( "zm_cmd_openmenu", ZM_Cmd_OpenMenu, "", FCVAR_HIDDEN );


void ZM_Cmd_CloseMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    pPlayer->SetMenuEnt( 0 );
}

static ConCommand zm_cmd_closemenu( "zm_cmd_closemenu", ZM_Cmd_CloseMenu, "", FCVAR_HIDDEN );


/*
    Phys explosion
*/
extern CUtlVector<CZMEntTriggerBlockPhysExp*> g_ZMBlockPhysExp;

ConVar zm_sv_physexp_cost( "zm_sv_physexp_cost", "400", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_physexp_radius( "zm_sv_physexp_radius", "222", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_physexp_magnitude( "zm_sv_physexp_magnitude", "17500", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_physexp_delay( "zm_sv_physexp_delay", "7.4", FCVAR_NOTIFY | FCVAR_ARCHIVE );

void ZM_Cmd_PhysExp( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    if ( !pPlayer->HasEnoughRes( zm_sv_physexp_cost.GetInt() ) )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
        return;
    }

    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) ) + 1.0f;

    if ( UTIL_PointContents( pos ) & CONTENTS_SOLID )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMPhysExpBlocked" );
        return;
    }


    CZMEntTriggerBlockPhysExp* pBlock;
    for ( int i = 0; i < g_ZMBlockPhysExp.Count(); i++ )
    {
        pBlock = g_ZMBlockPhysExp.Element( i );

        if (pBlock && pBlock->IsActive()
        &&  pBlock->CollisionProp()
        &&  pBlock->CollisionProp()->IsPointInBounds( pos ) )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMPhysExpBlocked" );
            return;
        }
    }


    CZMPhysExplosion::CreatePhysExplosion(
        pos,
        zm_sv_physexp_delay.GetFloat(),
        zm_sv_physexp_magnitude.GetFloat(),
        zm_sv_physexp_radius.GetFloat() );


    pPlayer->IncResources( -zm_sv_physexp_cost.GetInt() );


    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedPhysExp" );
}

static ConCommand zm_cmd_physexp( "zm_cmd_physexp", ZM_Cmd_PhysExp, "", FCVAR_HIDDEN );


/*
    Set selected zombies' mode
*/
void ZM_Cmd_SetZombieMode( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    ZombieMode_t mode = (ZombieMode_t)atoi( args.Arg( 1 ) );

    if ( mode <= ZOMBIEMODE_INVALID || mode >= ZOMBIEMODE_MAX )
        return;


    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ mode ]( CZMBaseZombie* pZombie )
    {
        if ( pZombie->GetZombieMode() == ZOMBIEMODE_AMBUSH )
        {
            pZombie->RemoveFromAmbush();
        }

        pZombie->SetZombieMode( mode );
    } );


    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM,
        ( mode == ZOMBIEMODE_OFFENSIVE ) ? "#ZMSetZombieModeOff" : "#ZMSetZombieModeDef" );
}

static ConCommand zm_cmd_zombiemode( "zm_cmd_zombiemode", ZM_Cmd_SetZombieMode, "" );


/*
    Set banshee ceiling cling.
*/
void ZM_Cmd_SetBansheeCeil( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    bool bSet = false;

    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ pPlayer, &bSet ]( CZMBaseZombie* pZombie )
    {
        if ( pZombie->GetZombieClass() != ZMCLASS_BANSHEE )
            return;


        bSet = true;

        static_cast<CZMBanshee*>( pZombie )->StartCeilingAmbush( pPlayer );
    } );

    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, bSet ? "#ZMBansheeCeilSet" : "#ZMBansheeCeilNoSel" );
}

static ConCommand zm_cmd_bansheeceiling( "zm_cmd_bansheeceiling", ZM_Cmd_SetBansheeCeil, "" );


/*
    Set zombie ambush.
*/
void ZM_Cmd_CreateAmbush( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    if ( !ZMUtil::GetSelectedZombieCount( pPlayer->entindex() ) )
        return;
    

    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) );

    if ( UTIL_PointContents( pos ) & CONTENTS_SOLID )
        return;


    CZMEntAmbushTrigger* pTrigger = dynamic_cast<CZMEntAmbushTrigger*>( CreateEntityByName( "info_ambush_trigger" ) );

    if ( !pTrigger ) return;

    if ( DispatchSpawn( pTrigger ) != 0 )
    {
        UTIL_RemoveImmediate( pTrigger );
        return;
    }

    pTrigger->Teleport( &pos, nullptr, nullptr );
    pTrigger->Activate();


    int count = 0;

    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ &count, pTrigger ]( CZMBaseZombie* pZombie )
    {
        ++count;


        pZombie->RemoveFromAmbush();

        pZombie->SetAmbush( pTrigger );
    } );

    pTrigger->SetAmbushZombies( count );



    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMSetZombieModeAmb" );
}

static ConCommand zm_cmd_createambush( "zm_cmd_createambush", ZM_Cmd_CreateAmbush, "" );


/*
    Move zombies to a line.
*/
void ZM_Cmd_MoveToLine( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 7 ) return;


    int nSelected = ZMUtil::GetSelectedZombieCount( pPlayer->entindex() );
    if ( !nSelected )
        return;
    

    Vector mypos = pPlayer->EyePosition();

    bool bIsOutsideWorld = (UTIL_PointContents( mypos ) & CONTENTS_SOLID) != 0;

    Vector start, end;
    start.x = atof( args.Arg( 1 ) );
    start.y = atof( args.Arg( 2 ) );
    start.z = atof( args.Arg( 3 ) ) + 1.0f;
    end.x = atof( args.Arg( 4 ) );
    end.y = atof( args.Arg( 5 ) );
    end.z = atof( args.Arg( 6 ) ) + 1.0f;
    
    Vector dir = end - start;
    float flLineLength = VectorNormalize( dir );
    

    // The line length is way too small to send the zombies into a line.
    if ( nSelected < 2 || flLineLength < 8.0f )
    {
        ZMUtil::MoveSelectedZombies( pPlayer->entindex(), start );
        return;
    }


    // Move selected zombies into a sorted list, from closest to farthest.
    CUtlVector<CZMBaseZombie*> vZombies;

    g_ZombieManager.ForEachSelectedZombie( pPlayer, [ &vZombies, &start ]( CZMBaseZombie* pZombie )
    {
        int j = 0;
        int len = vZombies.Count();
        for (; j < len; j++ )
        {
            // Is current zombie closer?
            if ( pZombie->GetAbsOrigin().DistToSqr( start ) < vZombies[j]->GetAbsOrigin().DistToSqr( start ) )
                break;
        }

        if ( j != len )
        {
            vZombies.InsertBefore( j, pZombie );
        }
        else
        {
            vZombies.AddToTail( pZombie );
        }
    } );


    Assert( nSelected == vZombies.Count() );


    trace_t trace;
    Vector walk = start;
    Vector add = dir * ( flLineLength / (vZombies.Count()-1) );

    for ( int i = 0; i < vZombies.Count(); i++ )
    {
        Vector command = walk;

        // Trace a line to find the exact spot we should be sending the unit.
        // This fixes any troubles caused by displacements and other non-flat surfaces.
        if ( !bIsOutsideWorld )
        {
            // Previous mask would hit players/zombies, etc.
            const unsigned int mask = CONTENTS_SOLID | CONTENTS_GRATE | CONTENTS_MONSTERCLIP;

            UTIL_TraceLine( mypos, walk, mask, pPlayer, COLLISION_GROUP_NONE, &trace );

            if ( !trace.startsolid )
                command = trace.endpos + Vector( 0.0f, 0.0f, 1.0f );
        }


        vZombies[i]->Command( pPlayer, command );

        walk += add;
    }
}

static ConCommand zm_cmd_moveline( "zm_cmd_moveline", ZM_Cmd_MoveToLine, "", FCVAR_HIDDEN );
