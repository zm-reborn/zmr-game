#include "cbase.h"

#ifdef CLIENT_DLL
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/npcs/zmr_zombiebase.h"
#endif

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_shareddefs.h"


#ifdef CLIENT_DLL
#define CZMBaseZombie C_ZMBaseZombie
#endif

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

CZMPlayer* CZMBaseZombie::GetSelector()
{
    return ToZMPlayer( UTIL_PlayerByIndex( m_iSelectorIndex ) );
}

int CZMBaseZombie::GetSelectorIndex()
{
    return m_iSelectorIndex;
}

void CZMBaseZombie::SetSelector( CZMPlayer* pPlayer )
{
#ifndef CLIENT_DLL
    int index = 0;

    if ( pPlayer ) index = pPlayer->entindex();


    SetSelector( index );
#endif
}

void CZMBaseZombie::SetSelector( int index )
{
#ifndef CLIENT_DLL
    m_iSelectorIndex = index;
#endif
}
