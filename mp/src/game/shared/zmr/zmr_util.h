#pragma once


#include "cbase.h"


namespace ZMClientUtil
{
#ifdef CLIENT_DLL
    bool WorldToScreen( const Vector& world, Vector& screen, int& x, int& y );
#endif
};
