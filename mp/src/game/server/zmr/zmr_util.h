#pragma once


#include "zmr_shareddefs.h"


namespace ZMUtil
{
    int GetSelectedZombieCount( int iPlayerIndex );
    void MoveSelectedZombies( int iPlayerIndex, const Vector& vecPos );

    void PrintNotify( CBasePlayer* pPlayer, ZMChatNotifyType_t type, const char* msg );
    void PrintNotifyAll( ZMChatNotifyType_t type, const char* msg );
    void SendNotify( IRecipientFilter& filter, ZMChatNotifyType_t type, const char* msg );
};
