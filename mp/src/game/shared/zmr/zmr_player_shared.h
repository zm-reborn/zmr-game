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


class CZMPlayerAttackTraceFilter : public CTraceFilter
{
public:
    CZMPlayerAttackTraceFilter( CBaseEntity* pAttacker, CBaseEntity* pIgnore, int collisionGroup );

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE;

private:
    CBaseEntity* m_pAttacker;
    CBaseEntity* m_pIgnore;
};
