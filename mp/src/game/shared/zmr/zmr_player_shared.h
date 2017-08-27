#pragma once



#ifndef CLIENT_DLL
#include "zmr/zmr_player.h"
#else
#include "zmr/c_zmr_player.h"
#endif

#ifdef CLIENT_DLL
#define CZMPlayer C_ZMPlayer
#else
#define C_ZMPlayer CZMPlayer
#endif
