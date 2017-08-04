#pragma once

#ifdef CLIENT_DLL
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/npcs/zmr_zombiebase.h"

#include "zmr/zmr_entities.h"
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

#ifndef CLIENT_DLL
    CUtlVector<CZMEntTriggerBlockHidden*> m_BlockHidden;
#endif
};

extern ZombieList_t* g_pZombies;

#ifndef CLIENT_DLL
extern CUtlVector<CZMEntTriggerBlockHidden*>* g_pBlockHidden;
#endif