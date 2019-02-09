#include "cbase.h"


#include "zmr_zombiebase_shared.h"

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr/npcs/zmr_zombieanimstate.h"



ConVar zm_sv_popcost_shambler( "zm_sv_popcost_shambler", "1", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_banshee( "zm_sv_popcost_banshee", "6", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_hulk( "zm_sv_popcost_hulk", "5", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_drifter( "zm_sv_popcost_drifter", "2", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_immolator( "zm_sv_popcost_immolator", "8", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );

ConVar zm_sv_cost_shambler( "zm_sv_cost_shambler", "10", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_banshee( "zm_sv_cost_banshee", "60", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_hulk( "zm_sv_cost_hulk", "70", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_drifter( "zm_sv_cost_drifter", "35", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_immolator( "zm_sv_cost_immolator", "100", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );


ConVar zm_sv_spawndelay_shambler( "zm_sv_spawndelay_shambler", "0.5", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_spawndelay_banshee( "zm_sv_spawndelay_banshee", "0.7", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_spawndelay_hulk( "zm_sv_spawndelay_hulk", "0.75", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_spawndelay_drifter( "zm_sv_spawndelay_drifter", "0.6", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_spawndelay_immolator( "zm_sv_spawndelay_immolator", "0.75", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );


ConVar zm_sv_happyzombies( "zm_sv_happyzombies", "0", FCVAR_REPLICATED, "Happy, happy zombies :)" );



CZMZombieManager g_ZombieManager;


ConVar zm_sv_debug_zombieik( "zm_sv_debug_zombieik", "0", FCVAR_REPLICATED );


ZombieClass_t CZMBaseZombie::NameToClass( const char* name )
{
    if ( Q_strnicmp( name, "npc_zombie", 10 ) == 0 )
    {
        return ZMCLASS_SHAMBLER;
    }

    if ( Q_strnicmp( name, "npc_fastzombie", 14 ) == 0 )
    {
        return ZMCLASS_BANSHEE;
    }

    if ( Q_strnicmp( name, "npc_poisonzombie", 16 ) == 0 )
    {
        return ZMCLASS_HULK;
    }

    if ( Q_strnicmp( name, "npc_dragzombie", 14 ) == 0 )
    {
        return ZMCLASS_DRIFTER;
    }

    if ( Q_strnicmp( name, "npc_burnzombie", 14 ) == 0 )
    {
        return ZMCLASS_IMMOLATOR;
    }

    return ZMCLASS_INVALID;
}

bool CZMBaseZombie::IsValidClass( ZombieClass_t zclass )
{
    return ( zclass > ZMCLASS_INVALID && zclass < ZMCLASS_MAX );
}

const char* CZMBaseZombie::ClassToName( ZombieClass_t zclass )
{
    switch ( zclass )
    {
    case ZMCLASS_SHAMBLER : return "npc_zombie";
    case ZMCLASS_BANSHEE : return "npc_fastzombie";
    case ZMCLASS_HULK : return "npc_poisonzombie";
    case ZMCLASS_DRIFTER : return "npc_dragzombie";
    case ZMCLASS_IMMOLATOR : return "npc_burnzombie";
    default : return nullptr;
    }
}

int CZMBaseZombie::GetPopCost( ZombieClass_t zclass )
{
    switch ( zclass )
    {
    case ZMCLASS_SHAMBLER : return zm_sv_popcost_shambler.GetInt();
    case ZMCLASS_BANSHEE : return zm_sv_popcost_banshee.GetInt();
    case ZMCLASS_HULK : return zm_sv_popcost_hulk.GetInt();
    case ZMCLASS_DRIFTER : return zm_sv_popcost_drifter.GetInt();
    case ZMCLASS_IMMOLATOR : return zm_sv_popcost_immolator.GetInt();
    default : return 0;
    }
}

int CZMBaseZombie::GetCost( ZombieClass_t zclass )
{
    switch ( zclass )
    {
    case ZMCLASS_SHAMBLER : return zm_sv_cost_shambler.GetInt();
    case ZMCLASS_BANSHEE : return zm_sv_cost_banshee.GetInt();
    case ZMCLASS_HULK : return zm_sv_cost_hulk.GetInt();
    case ZMCLASS_DRIFTER : return zm_sv_cost_drifter.GetInt();
    case ZMCLASS_IMMOLATOR : return zm_sv_cost_immolator.GetInt();
    default : return 0;
    }
}

float CZMBaseZombie::GetSpawnDelay( ZombieClass_t zclass )
{
    switch ( zclass )
    {
    case ZMCLASS_SHAMBLER : return zm_sv_spawndelay_shambler.GetFloat();
    case ZMCLASS_BANSHEE : return zm_sv_spawndelay_banshee.GetFloat();
    case ZMCLASS_HULK : return zm_sv_spawndelay_hulk.GetFloat();
    case ZMCLASS_DRIFTER : return zm_sv_spawndelay_drifter.GetFloat();
    case ZMCLASS_IMMOLATOR : return zm_sv_spawndelay_immolator.GetFloat();
    default : return zm_sv_spawndelay_shambler.GetFloat();
    }
}

bool CZMBaseZombie::HasEnoughPopToSpawn( ZombieClass_t zclass )
{
    CZMRules* pRules = ZMRules();


    int curpop = 0;

    if ( pRules )
    {
        curpop = pRules->GetZombiePop();
    }


    return (curpop + GetPopCost( zclass )) <= zm_sv_zombiemax.GetInt();
}

CZMPlayer* CZMBaseZombie::GetSelector() const
{
    return ToZMPlayer( UTIL_PlayerByIndex( m_iSelectorIndex ) );
}

int CZMBaseZombie::GetSelectorIndex() const
{
    return m_iSelectorIndex;
}

void CZMBaseZombie::SetSelector( CZMPlayer* pPlayer )
{
    int index = 0;

    if ( pPlayer ) index = pPlayer->entindex();


    SetSelector( index );
}

void CZMBaseZombie::SetSelector( int index )
{
    m_iSelectorIndex = index;
}

ZombieClass_t CZMBaseZombie::GetZombieClass() const
{
    return m_iZombieClass;
}

void CZMBaseZombie::SetZombieClass( ZombieClass_t zclass )
{
    m_iZombieClass = zclass;
}

int CZMBaseZombie::GetPopCost() const
{
    return GetPopCost( GetZombieClass() );
}

int CZMBaseZombie::GetCost() const
{
    return GetCost( GetZombieClass() );
}

#ifndef CLIENT_DLL
void TE_ZombieAnimEvent( CZMBaseZombie* pZombie, ZMZombieAnimEvent_t anim, int nData );
#endif

bool CZMBaseZombie::DoAnimationEvent( int iEvent, int nData )
{
#ifdef CLIENT_DLL
    MDLCACHE_CRITICAL_SECTION();
#endif

#ifdef GAME_DLL
    // Depending on the event, send a random seed in data
    GetAnimRandomSeed( iEvent, nData );
#endif


    m_iAdditionalAnimRandomSeed = nData;

    bool ret = m_pAnimState->DoAnimationEvent( (ZMZombieAnimEvent_t)iEvent, nData );

#ifndef CLIENT_DLL
    TE_ZombieAnimEvent( this, (ZMZombieAnimEvent_t)iEvent, nData );
#endif

    m_iAdditionalAnimRandomSeed = 0;

    return ret;
}

int CZMBaseZombie::GetAnimationRandomSeed()
{
    return m_iAnimationRandomSeed + m_iAdditionalAnimRandomSeed;
}

bool CZMBaseZombie::IsPlayerControlled() const
{
    return m_iPlayerControllerIndex != 0;
}

int CZMBaseZombie::GetControllerIndex() const
{
    return m_iPlayerControllerIndex;
}

bool CZMBaseZombie::CanBePenetrated() const
{
    return true;
}
