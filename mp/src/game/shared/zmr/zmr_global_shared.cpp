#include "cbase.h"

#include "zmr_global_shared.h"


CZMGlobalLists::CZMGlobalLists()
{
#ifndef CLIENT_DLL
    m_BlockHidden.Purge();
    m_BlockPhysExp.Purge();
#endif
}

CZMGlobalLists::~CZMGlobalLists()
{
#ifndef CLIENT_DLL
    m_BlockHidden.Purge();
    m_BlockPhysExp.Purge();
#endif
}

CZMGlobalLists* g_pZMGlobals = new CZMGlobalLists();


#ifndef CLIENT_DLL
CUtlVector<CZMEntTriggerBlockHidden*>* g_pBlockHidden = &(g_pZMGlobals->m_BlockHidden);
CUtlVector<CZMEntTriggerBlockPhysExp*>* g_pBlockPhysExp = &(g_pZMGlobals->m_BlockPhysExp);
#endif
