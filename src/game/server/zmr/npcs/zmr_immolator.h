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


    virtual void Precache() override;
    virtual void Spawn() override;

    virtual void HandleAnimEvent( animevent_t* pEvent ) override;

    virtual void PreUpdate() override;

    virtual void Ignite( float flFlameLifetime, bool bNPCOnly = false, float flSize = 0.0f, bool bCalledByLevelDesigner = false ) override;
    virtual void Event_Killed( const CTakeDamageInfo& info ) override;


    virtual bool ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const override;

    virtual bool ShouldPlayIdleSound() const override;
    virtual float IdleSound() override;
    virtual void AlertSound() override;
    virtual void DeathSound() override;


private:
    void StartFires();


    void BurnHurtOthers();
    float m_flNextBurnDamageTime;
};
