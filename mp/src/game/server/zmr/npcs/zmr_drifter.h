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


    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;


    // Drifter doesn't get any damage changes.
    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const OVERRIDE { return false; }


    virtual float GetClawAttackRange() const OVERRIDE { return 112.0f; }
    virtual void GetAttackHull( Vector& mins, Vector& maxs ) const OVERRIDE;
    virtual float GetAttackLowest() const OVERRIDE { return -20.0f; }


    virtual bool ShouldPlayIdleSound() const OVERRIDE;
    virtual float IdleSound() OVERRIDE;
    virtual void AlertSound() OVERRIDE;
    virtual void AttackSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;


    static int AE_DRAGGY_SICK;
};
