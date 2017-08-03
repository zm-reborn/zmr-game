#include "cbase.h"

#include "zmr_global_shared.h"


CZMGlobalLists::CZMGlobalLists()
{
    m_Zombies.Purge();
}

CZMGlobalLists::~CZMGlobalLists()
{
    m_Zombies.Purge();
}

CZMGlobalLists* g_pZMGlobals = new CZMGlobalLists();

ZombieList_t* g_pZombies = &(g_pZMGlobals->m_Zombies);