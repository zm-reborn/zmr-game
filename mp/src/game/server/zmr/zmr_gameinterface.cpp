#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"

#include "zmr_mapentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


/*
    NOTE: Remove from project
    hl2mp/hl2mp_gameinterface.cpp
    hl2mp/hl2mp_gameinterface.h

*/

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
    minplayers = 2;
    defaultMaxPlayers = 16;
    maxplayers = 32;
}

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
    g_ZMMapEntities.InitialSpawn( pMapEntities );
}
