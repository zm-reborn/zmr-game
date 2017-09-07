#pragma once


#include "cbase.h"


#ifdef CLIENT_DLL
namespace ZMClientUtil
{
    int ShowTooltipByName( const char* );
    void ShowTooltip( const char* );
    void HideTooltip( int index );
    void HideTooltip();

    bool WorldToScreen( const Vector& world, Vector& screen, int& x, int& y );
    int GetSelectedZombieCount();
};
#endif
