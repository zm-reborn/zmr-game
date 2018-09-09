#include "cbase.h"

#include "zmr_scuffler.h"
#include "zmr/zmr_gamerules.h"
#include "npcs/zmr_zombiebase.h"


CZMScufflerSystem g_ZMScufflerSystem;


bool CZMScufflerSystem::OnRequestScuffler( CZMPlayer* pPlayer )
{
    CZMRules* pRules = ZMRules();
    if ( !pRules )
        return false;


    if ( pPlayer->IsControllingZombie() )
    {
        CZMBaseZombie* pZombie = ToZMBaseZombie( pPlayer->GetObserverTarget() );
        if ( pZombie )
        {
            pZombie->SetPlayerControlled( nullptr );
        }

        return false;
    }

    if ( pPlayer->m_flNextScufflerTime > gpGlobals->curtime )
        return false;

    if ( pPlayer->GetTeamNumber() != ZMTEAM_SPECTATOR )
        return false;

    if ( ZMRules() && ZMRules()->IsInRoundEnd() )
        return false;

    CZMEntZombieSpawn* pSpawn = FindClosestZombieSpawn( pPlayer->GetAbsOrigin() );
    if ( !pSpawn )
        return false;


    CZMBaseZombie* pZombie = pSpawn->CreateZombie( ZMCLASS_SHAMBLER );
    if ( !pZombie )
        return false;


    pPlayer->SetObserverTarget( pZombie );
    if ( pPlayer->GetObserverMode() != OBS_MODE_CHASE )
        pPlayer->SetObserverMode( OBS_MODE_CHASE );
    //pPlayer->SetObserverMode( OBS_MODE_CHASE );
    pZombie->SetPlayerControlled( pPlayer );


    pPlayer->m_flNextScufflerTime = gpGlobals->curtime + 20.0f;


    if ( pRules )
    {
        pRules->SetZombiePop( pRules->GetZombiePop() - pZombie->GetPopCost() );
    }

    return true;
}

CZMEntZombieSpawn* CZMScufflerSystem::FindClosestZombieSpawn( const Vector& vecPos, float flMaxDist ) const
{
    float closestDistSqr = flMaxDist * flMaxDist;
    CZMEntZombieSpawn* pClosest = nullptr;

    CZMEntZombieSpawn* pEnt = nullptr;
    while ( (pEnt = static_cast<CZMEntZombieSpawn*>( gEntList.FindEntityByClassname( pEnt, "info_zombiespawn" ) )) != nullptr )
    {
        if ( !pEnt->IsActive() )
            continue;

        if ( !pEnt->CanSpawn( ZMCLASS_SHAMBLER ) )
            continue;

        float dist = vecPos.DistToSqr( pEnt->GetAbsOrigin() );
        if ( dist < closestDistSqr )
        {
            closestDistSqr = dist;
            pClosest = pEnt;
        }
    }

    return pClosest;
}


CON_COMMAND( zm_zombie_scuffler, "" )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );
    if ( !pPlayer ) return;


    g_ZMScufflerSystem.OnRequestScuffler( pPlayer );
}
