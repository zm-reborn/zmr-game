#pragma once


#include "zmr_zombiebase.h"

class CZMImmolator : public CZMBaseZombie
{
public:
    typedef CZMImmolator ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMImmolator, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMImmolator();
    ~CZMImmolator();


    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual void PreUpdate() OVERRIDE;

    virtual void Ignite( float flFlameLifetime, bool bNPCOnly = false, float flSize = 0.0f, bool bCalledByLevelDesigner = false ) OVERRIDE;
    virtual void Event_Killed( const CTakeDamageInfo& info ) OVERRIDE;


    virtual void AlertSound() OVERRIDE;
    virtual void AttackSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;


private:
    void StartFires();
};
