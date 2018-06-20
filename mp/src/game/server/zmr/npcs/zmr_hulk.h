#pragma once



#include "zmr_zombiebase.h"

class CZMHulkMotor : public NPCR::CNonPlayerMotor
{
public:
    CZMHulkMotor( CZMBaseZombie* pOuter ) : NPCR::CNonPlayerMotor( pOuter )
    {
    }

    virtual float GetHullHeight() const OVERRIDE { return 100.0f; }

    //virtual float GetFrictionForward() const OVERRIDE { return IsAttemptingToMove() ? 0.0f : 0.2f; }
};

class CZMHulk : public CZMBaseZombie
{
public:
    typedef CZMHulk ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMHulk, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMHulk();
    ~CZMHulk();


    virtual CZMHulkMotor* CreateMotor() OVERRIDE { return new CZMHulkMotor( this ); }


    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;


    virtual NPCR::CPathCostGroundOnly*  GetPathCost() const OVERRIDE;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const OVERRIDE;


    virtual float GetClawAttackRange() const OVERRIDE { return 80.0f; }
    virtual float GetAttackHeight() const OVERRIDE { return CollisionProp()->OBBMaxs().z + 10.0f; }


    virtual bool ShouldPlayIdleSound() const OVERRIDE;
    virtual float IdleSound() OVERRIDE;
    virtual void AlertSound() OVERRIDE;
    virtual void AttackSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;
};
