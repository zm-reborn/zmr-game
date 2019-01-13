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


class CZMPlayerAttackTraceFilter : public CTraceFilter
{
public:
    CZMPlayerAttackTraceFilter( CBaseEntity* pAttacker, CBaseEntity* pIgnore, int collisionGroup );

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE;


    int     AddToIgnoreList( CBaseEntity* pIgnore );
    bool    RemoveFromIgnoreList( CBaseEntity* pIgnore );

private:
    CBaseEntity* m_pAttacker;

    CUtlVector<CBaseEntity*> m_vIgnore;
};

struct ZMFireBulletsInfo_t
{
    ZMFireBulletsInfo_t( const FireBulletsInfo_t& bulletinfo, Vector vecDir, CZMPlayerAttackTraceFilter* filter ) : info( bulletinfo ), Manipulator( vecDir ), pFilter( filter )
    {
        bStartedInWater = false;
        bDoImpacts = false;
        bDoTracers = false;
        flCumulativeDamage = 0.0f;
        this->vecDir = vecDir;
    }

    int iShot;
    int iPlayerDamage;

    bool bStartedInWater;
    bool bDoImpacts;
    bool bDoTracers;
    float flCumulativeDamage;

    const FireBulletsInfo_t& info;
    CShotManipulator Manipulator;
    CZMPlayerAttackTraceFilter* pFilter;
    Vector vecDir;
};
