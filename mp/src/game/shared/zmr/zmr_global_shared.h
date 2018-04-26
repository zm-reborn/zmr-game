#pragma once


#include "zmr/npcs/zmr_zombiebase_shared.h"

#ifndef CLIENT_DLL
#include "zmr/zmr_entities.h"
#endif

#ifdef CLIENT_DLL
#define CZMGlobalLists C_ZMGlobalLists
#endif

class CZMGlobalLists
{
public:
    //DECLARE_CLASS( CZMGlobalLists )

    CZMGlobalLists();
    ~CZMGlobalLists();

#ifndef CLIENT_DLL
    CUtlVector<CZMEntTriggerBlockHidden*> m_BlockHidden;
    CUtlVector<CZMEntTriggerBlockPhysExp*> m_BlockPhysExp;
#endif
};

#ifndef CLIENT_DLL
extern CUtlVector<CZMEntTriggerBlockHidden*>* g_pBlockHidden;
extern CUtlVector<CZMEntTriggerBlockPhysExp*>* g_pBlockPhysExp;
#endif
