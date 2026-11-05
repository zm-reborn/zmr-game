#pragma once


#include "basegrenade_shared.h"

class CZMBaseProjectile : public CBaseGrenade
{
public:
    DECLARE_CLASS( CZMBaseProjectile, CBaseGrenade );

    CZMBaseProjectile();

    virtual void OnThrow() {};

    virtual void Detonate() override;

    float GetDetonationTime() const { return m_flDetonateTime; }
    void SetDetonationTime( float t ) { m_flDetonateTime = t; }

    virtual void Spawn() override;
    void ArmedDetonateThink();

protected:
    void PostExplode();
};
