#pragma once


#include "mapentities.h"


class CZMMapEntitiesSystem
{
public:
    CZMMapEntitiesSystem();
    ~CZMMapEntitiesSystem();


    void InitialSpawn( const char* pMapEntities );
    void Restore();


    static bool IsDebugging();
    static bool IsPreservedClassname( const char* pszClassname );


    bool ShouldCreateEntity( const char* pszClassname, CEntityMapData& entData );
    CBaseEntity* CreateEntity( const char* pszClassname );

private:
    static const char** GetPreserveEntityList();

    CBaseEntity* BuildListCreate( const char* pszClassname );



    int m_iIterator;
    bool m_bBuildRefList;
};

extern CZMMapEntitiesSystem g_ZMMapEntities;
