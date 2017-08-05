#include "cbase.h"


#include "zmr_player_shared.h"

#ifdef CLIENT_DLL
#include "zmr/c_zmr_player.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/zmr_player.h"
#include "zmr/npcs/zmr_zombiebase.h"
#endif


#ifdef CLIENT_DLL
//#define CZMPlayer C_ZMPlayer
#define CZMBaseZombie C_ZMBaseZombie
#endif


bool CZMPlayer::HasEnoughResToSpawn( ZombieClass_t zclass )
{
    return GetResources() >= CZMBaseZombie::GetCost( zclass );
}

bool CZMPlayer::HasEnoughRes( int cost )
{
    return GetResources() >= cost;
}

int CZMPlayer::GetResources()
{
    return m_nResources;
}

void CZMPlayer::SetResources( int res )
{
#ifdef CLIENT_DLL

#else
    m_nResources = res;
#endif
}

// This + AllowsAutoSwitchFrom was causing problems with switching to an empty weapon.
bool CZMPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
    if ( !IsAlive() )
        return false;

    if ( !pWeapon->CanDeploy() )
        return false;
    

    CBaseCombatWeapon* pActive = GetActiveWeapon();
    if ( pActive && !pActive->CanHolster() )
    {
        return false;
    }

    return true;
}