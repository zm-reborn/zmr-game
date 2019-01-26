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


#include "shot_manipulator.h"


struct ZMFireBulletsInfo_t;

class CZMPlayerAttackTraceFilter : public CTraceFilter
{
public:
    CZMPlayerAttackTraceFilter( CBaseEntity* pAttacker, CBaseEntity* pIgnore, int collisionGroup );

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE;


    int     AddToIgnoreList( CBaseEntity* pIgnore );
    bool    RemoveFromIgnoreList( CBaseEntity* pIgnore );
    void    ClearIgnoreList();

    int     GetPenetrations() const { return m_nPenetrations; }
    void    ClearPenetrations() { m_nPenetrations = 0; }

private:
    CBaseEntity* m_pAttacker;

    CUtlVector<CBaseEntity*> m_vIgnore;

    int m_nPenetrations;
};
