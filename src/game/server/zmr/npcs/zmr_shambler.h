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

    virtual void Precache() override;
    virtual void Spawn() override;

    virtual void    HandleAnimEvent( animevent_t* pEvent ) override;

    virtual bool        CanBreakObject( CBaseEntity* pEnt, bool bSwat = false ) const override;
    virtual bool        CanSwatPhysicsObjects() const override { return true; }
    virtual Activity    GetSwatActivity( CBaseEntity* pEnt, bool bBreak = true ) const override;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const override;


    virtual bool ShouldPlayIdleSound() const override;
    virtual float IdleSound() override;
    virtual float PainSound( const CTakeDamageInfo& info ) override;
    virtual void AlertSound() override;
    virtual void DeathSound() override;
};
