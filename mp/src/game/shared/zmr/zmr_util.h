#pragma once


#include "cbase.h"


#ifdef CLIENT_DLL
namespace ZMClientUtil
{
    bool WorldToScreen( const Vector& world, Vector& screen, int& x, int& y );
    int GetSelectedZombieCount();
};
#endif
