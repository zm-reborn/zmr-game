#pragma once


#include "cbase.h"


#ifdef CLIENT_DLL
#define C_ZMPlayer CZMPlayer
#else
#define C_ZMPlayer CZMPlayer
#endif

#ifndef CLIENT_DLL
#include "zmr/zmr_player.h"
#else
#include "zmr/c_zmr_player.h"
#endif

/*
inline C_ZMPlayer* ToZMPlayer( CBaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CZMPlayer*>( pEntity );
}

inline C_ZMPlayer* ToZMPlayer( CBasePlayer* pPlayer )
{
	return static_cast<CZMPlayer*>( pPlayer );
}*/