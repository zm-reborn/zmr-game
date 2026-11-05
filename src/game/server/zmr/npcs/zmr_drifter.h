#pragma once



#include "zmr_zombiebase.h"

class CZMDrifter : public CZMBaseZombie
{
public:
    typedef CZMDrifter ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMDrifter, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMDrifter();
    ~CZMDrifter();


    virtual void Precache() override;
    virtual void Spawn() override;

    virtual void HandleAnimEvent( animevent_t* pEvent ) override;


    // Drifter doesn't get any damage changes.
    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const override { return false; }


    virtual float GetClawAttackRange() const override { return 112.0f; }
    virtual void GetAttackHull( Vector& mins, Vector& maxs ) const override;
    virtual float GetAttackLowest() const override { return -20.0f; }


    virtual bool ShouldPlayIdleSound() const override;
    virtual float IdleSound() override;
    virtual void AlertSound() override;
    virtual void DeathSound() override;


    static int AE_DRAGGY_SICK;
};
