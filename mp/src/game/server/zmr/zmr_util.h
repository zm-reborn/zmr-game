#pragma once


#include "zmr_shareddefs.h"


namespace ZMUtil
{
    int GetSelectedZombieCount( int iPlayerIndex );
    void MoveSelectedZombies( int iPlayerIndex, const Vector& vecPos );

    void PrintNotify( CBasePlayer* pPlayer, ZMChatNotifyType_t type, const char* msg );
    void PrintNotifyAll( ZMChatNotifyType_t type, const char* msg );
    void SendNotify( IRecipientFilter& filter, ZMChatNotifyType_t type, const char* msg );

    inline int CountPlayers( bool bNoBots = false, int team = TEAM_ANY, bool bAlive = false )
    {
        int num = 0;

        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            auto* pPlayer = UTIL_PlayerByIndex( i );

            if ( !pPlayer || !pPlayer->IsPlayer() )
                continue;

            if ( bNoBots && pPlayer->IsBot() )
                continue;

            if ( !pPlayer->IsConnected() )
                continue;

            if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
                continue;

            if ( bAlive && !pPlayer->IsAlive() )
                continue;

            ++num;
        }

        return num;
    }
};
