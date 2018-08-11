#pragma once



#include "zmr_zombiebase.h"

class CZMShambler : public CZMBaseZombie
{
public:
    typedef CZMShambler ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMShambler, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMShambler();
    ~CZMShambler();

    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void    HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual bool        CanBreakObject( CBaseEntity* pEnt, bool bSwat = false ) const OVERRIDE;
    virtual bool        CanSwatPhysicsObjects() const OVERRIDE { return true; }
    virtual Activity    GetSwatActivity( CBaseEntity* pEnt, bool bBreak = true ) const OVERRIDE;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const OVERRIDE;


    virtual bool ShouldPlayIdleSound() const OVERRIDE;
    virtual float IdleSound() OVERRIDE;
    virtual void AlertSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
};
