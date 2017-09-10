#pragma once


#include "cbase.h"


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
};
#endif
