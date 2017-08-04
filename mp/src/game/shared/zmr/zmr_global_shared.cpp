#include "cbase.h"

#include "zmr_global_shared.h"


CZMGlobalLists::CZMGlobalLists()
{
    m_Zombies.Purge();

#ifndef CLIENT_DLL
    m_BlockHidden.Purge();
#endif
}

CZMGlobalLists::~CZMGlobalLists()
{
    m_Zombies.Purge();

#ifndef CLIENT_DLL
    m_BlockHidden.Purge();
#endif
}

CZMGlobalLists* g_pZMGlobals = new CZMGlobalLists();

ZombieList_t* g_pZombies = &(g_pZMGlobals->m_Zombies);


#ifndef CLIENT_DLL
CUtlVector<CZMEntTriggerBlockHidden*>* g_pBlockHidden = &(g_pZMGlobals->m_BlockHidden);
#endif
