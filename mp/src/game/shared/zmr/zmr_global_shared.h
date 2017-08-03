#pragma once

#ifdef CLIENT_DLL
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/npcs/zmr_zombiebase.h"
#endif

#ifdef CLIENT_DLL
#define CZMGlobalLists C_ZMGlobalLists
#endif

#ifdef CLIENT_DLL
typedef CUtlVector<C_ZMBaseZombie*> ZombieList_t;
#else
typedef CUtlVector<CZMBaseZombie*> ZombieList_t;
#endif

class CZMGlobalLists
{
public:
    //DECLARE_CLASS( CZMGlobalLists )

    CZMGlobalLists();
    ~CZMGlobalLists();


    ZombieList_t m_Zombies;
};

extern ZombieList_t* g_pZombies;