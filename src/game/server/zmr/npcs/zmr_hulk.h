#pragma once



#include "zmr_zombiebase.h"

class CZMHulkMotor : public CZMBaseZombieMotor
{
public:
    CZMHulkMotor( CZMBaseZombie* pOuter ) : CZMBaseZombieMotor( pOuter )
    {
    }

    virtual float GetHullHeight() const override { return 100.0f; }

    //virtual float GetFrictionForward() const override { return IsAttemptingToMove() ? 0.0f : 0.2f; }
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


    virtual CZMHulkMotor* CreateMotor() override { return new CZMHulkMotor( this ); }


    virtual void Precache() override;
    virtual void Spawn() override;

    virtual void HandleAnimEvent( animevent_t* pEvent ) override;


    virtual NPCR::CPathCostGroundOnly*  GetPathCost() const override;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const override;


    virtual float GetClawAttackRange() const override { return 80.0f; }
    virtual float GetAttackHeight() const override { return CollisionProp()->OBBMaxs().z + 10.0f; }


    virtual bool ShouldPlayIdleSound() const override;
    virtual float IdleSound() override;
    virtual float PainSound( const CTakeDamageInfo& info ) override;
    virtual void AlertSound() override;
    virtual void DeathSound() override;


    // Shared
    virtual bool    CanBePenetrated() const override;
};
