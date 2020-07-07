#include "cbase.h"


#ifdef GAME_DLL
#include "zmr_util.h"
#include "zmr_entities.h"
#endif
#include "npcs/zmr_zombiebase_shared.h"
#include "zmr_player_shared.h"
#include "zmr_shareddefs.h"

#include "zmr_hiddenspawn.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#ifdef GAME_DLL
CUtlVector<CZMEntTriggerBlockHidden*> g_ZMBlockHidden;
#endif


ConVar zm_sv_hidden_cost_shambler( "zm_sv_hidden_cost_shambler", "100", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar zm_sv_hidden_mindistance( "zm_sv_hidden_mindistance", "144", FCVAR_NOTIFY | FCVAR_REPLICATED, "The closest distance (in units) to survivors that the ZM can spawn zombies in." );
ConVar zm_sv_hidden_zombiedistmult( "zm_sv_hidden_zombiedistmult", "256", FCVAR_NOTIFY | FCVAR_REPLICATED, "Zombie further away than this will have minimum cost." );
ConVar zm_sv_hidden_mincost( "zm_sv_hidden_mincost", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "The minimum amount a hidden spawn will cost." );



//
//
//
CZMHiddenSpawnSystem::CZMHiddenSpawnSystem()
{

}

CZMHiddenSpawnSystem::~CZMHiddenSpawnSystem()
{

}

float CZMHiddenSpawnSystem::GetMinimumDistance() const
{
    return zm_sv_hidden_mindistance.GetFloat();
}

int CZMHiddenSpawnSystem::GetResourcesMax( ZombieClass_t zclass ) const
{
    return zm_sv_hidden_cost_shambler.GetInt();
}

int CZMHiddenSpawnSystem::ComputeResourceCost( ZombieClass_t zclass, float closestZombieDist ) const
{
    const float flMaxDist = zm_sv_hidden_zombiedistmult.GetFloat();
    Assert( flMaxDist > 0.0f );


    float frac = 1.0f - RemapValClamped( closestZombieDist, 24.0f, flMaxDist, 0.0f, 1.0f );


    return zm_sv_hidden_mincost.GetInt() + frac * GetResourcesMax( zclass );
}

void CZMHiddenSpawnSystem::Trace( CZMPlayer* pSurvivor, const Vector& endpos, trace_t& tr ) const
{
    UTIL_TraceLine( pSurvivor->EyePosition(), endpos, MASK_VISIBLE, nullptr, COLLISION_GROUP_NONE, &tr );
}

bool CZMHiddenSpawnSystem::CanSee( CZMPlayer* pSurvivor, const Vector& zombiepos ) const
{
    trace_t tr;

    // Check if the fog obscures the zombie
    auto* pController = pSurvivor->m_Local.m_PlayerFog.m_hCtrl.Get();
    if ( pController && pController->m_fog.maxdensity > 0.9f )
    {
        float dist = pSurvivor->EyePosition().DistTo( zombiepos );
        if ( pController->m_fog.farz > 0.0f && dist > pController->m_fog.farz )
        {
            return false;
        }

        if ( pController->m_fog.end > 0.0f && dist > pController->m_fog.end )
        {
            return false;
        }
    }
    


    Trace( pSurvivor, zombiepos, tr );

    if ( tr.fraction == 1.0f )
    {
        return true;
    }

    
    Trace( pSurvivor, zombiepos + Vector( 0, 0, 64 ), tr );

    if ( tr.fraction == 1.0f )
    {
        return true;
    }

    return false;
}

bool CZMHiddenSpawnSystem::CanSpawnClass( ZombieClass_t zclass ) const
{
    return zclass == ZMCLASS_SHAMBLER;
}

HiddenSpawnError_t CZMHiddenSpawnSystem::Spawn( ZombieClass_t zclass, CZMPlayer* pZM, const Vector& pos, int* pResourceCost )
{
    if ( !CanSpawnClass( zclass ) )
    {
        return HSERROR_BADCLASS;
    }


    if ( !CZMBaseZombie::HasEnoughPopToSpawn( zclass ) )
    {
#ifdef GAME_DLL
        ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMNotEnoughPop" );
#endif
        return HSERROR_NOTENOUGHPOP;
    }


    //
    // Check for any block triggers
    //
#ifdef GAME_DLL
    CZMEntTriggerBlockHidden* pBlock;
    for ( int i = 0; i < g_ZMBlockHidden.Count(); i++ )
    {
        pBlock = g_ZMBlockHidden.Element( i );

        if (pBlock && pBlock->IsActive()
        &&  pBlock->CollisionProp()
        &&  pBlock->CollisionProp()->IsPointInBounds( pos ))
        {
            ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMHiddenCreateBlocked" );
            return HSERROR_BLOCKEDSPOT;
        }
    }
#endif


    //
    // Check visibility
    //
    const float flMinDist = GetMinimumDistance();
    const float flMinDistSqr = flMinDist * flMinDist;


    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( !pPlayer->IsHuman() ) continue;

        if ( !pPlayer->IsAlive() ) continue;


        Vector plypos = pPlayer->GetAbsOrigin();
        if ( plypos.DistToSqr( pos ) < flMinDistSqr )
        {
#ifdef GAME_DLL
            ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMHiddenCreateHumanClose" );
#endif
            return HSERROR_TOOCLOSE;
        }

        if ( CanSee( pPlayer, pos ) )
        {
#ifdef GAME_DLL
            ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMHiddenCreateHumanSee" );
#endif
            return HSERROR_CANSEE;
        }
    }


    //
    // Compute the resource cost
    //
    float closestZombie = FLT_MAX;

    g_ZombieManager.ForEachAliveZombie( [ &closestZombie, pos ]( CZMBaseZombie* pZombie )
    {
        float dist = pZombie->GetAbsOrigin().DistTo( pos );

        if ( dist < closestZombie )
            closestZombie = dist;
    } );



    int iResCost = ComputeResourceCost( zclass, closestZombie );

    if ( pResourceCost )
        *pResourceCost = iResCost;


    if ( !pZM->HasEnoughRes( iResCost ) )
    {
#ifdef GAME_DLL
        ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
#endif
        return HSERROR_NOTENOUGHRES;
    }


    //
    // Do the actual spawning
    //
#ifdef GAME_DLL
    auto* pZombie =
        static_cast<CZMBaseZombie*>( CreateEntityByName( CZMBaseZombie::ClassToName( zclass ) ) );

    if ( !pZombie ) return HSERROR_UNKNOWN;


    if ( DispatchSpawn( pZombie ) != 0 )
    {
        UTIL_RemoveImmediate( pZombie );
    }
    
    const Vector findground( 0.0f, 0.0f, 2.0f );

    Vector mins, maxs;
    maxs.x = pZombie->GetMotor()->GetHullWidth() / 2.0f;
    maxs.y = maxs.x;
    maxs.z = pZombie->GetMotor()->GetHullHeight();

    mins.x = mins.y = -maxs.x;
    mins.z = 0.0f;


    //
    // Trace down to get ground position.
    //
    trace_t tr;

    UTIL_TraceHull(
        pos + Vector( 0.0f, 0.0f, 8.0f ),
        pos - findground,
        mins, maxs,
        MASK_NPCSOLID, pZombie, COLLISION_GROUP_NONE, &tr );
    
    const Vector up = Vector( 0.0f, 0.0f, 1.0f );
    const float dot = DotProduct( up, tr.plane.normal );

    if (tr.startsolid || tr.fraction == 1.0f
    ||  (tr.fraction != 1.0f && dot < 0.6f)) // Is our spawn location ground?
    {
        ZMUtil::PrintNotify( pZM, ZMCHATNOTIFY_ZM, "#ZMInvalidSpot" );

        pZombie->SUB_Remove();

        return HSERROR_INVALIDSPOT;
    }

    
    pZombie->Teleport( &tr.endpos, nullptr, nullptr );

    // Face away from ZM.
    QAngle ang = pZM->GetAbsAngles();
    ang.x = ang.z = 0.0f;
    pZombie->SetAbsAngles( ang );

    pZombie->Activate();

    pZM->IncResources( -iResCost );
#endif

    return HSERROR_OK;
}
//
//
//

CZMHiddenSpawnSystem g_ZMHiddenSpawn;
