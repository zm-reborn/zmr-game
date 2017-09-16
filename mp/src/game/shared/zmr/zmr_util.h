#pragma once


#include "cbase.h"

#ifdef CLIENT_DLL
#include "zmr/npcs/c_zmr_zombiebase.h"
#endif

#ifdef CLIENT_DLL
namespace ZMClientUtil
{
    void ChatPrint( const char* format, ... );


    void QueueTooltip( const char*, float delay );
    int ShowTooltipByName( const char*, bool force = false );
    void ShowTooltip( const char* );
    void HideTooltip( int index );
    void HideTooltip();

    bool WorldToScreen( const Vector& world, Vector& screen, int& x, int& y );

    int GetSelectedZombieCount();

    void SelectAllZombies( bool bSendCommand = true );
    void DeselectAllZombies( bool bSendCommand = true );
    void SelectSingleZombie( C_ZMBaseZombie*, bool bSticky );
    void SelectZombies( const CUtlVector<C_ZMBaseZombie*>&, bool bSticky );
};
#endif
