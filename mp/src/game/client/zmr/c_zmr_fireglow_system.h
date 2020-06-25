#pragma once

#include "dlight.h"

typedef C_BaseEntity FireGlowEnt_t;

enum class FireGlowType_t
{
    GLOW_GENERIC_FIRE = 0,

    GLOW_MOLOTOV_FIRE,
    GLOW_IMMOLATOR
};


struct FireData_t
{
    FireData_t( FireGlowEnt_t* pEnt, FireGlowType_t glowType );
    ~FireData_t();

    void Update( float flUpdateInterval );
    void Kill();
    void Decay( float tim );
    bool DecaySingleEntity( float tim, FireGlowEnt_t* pEnt );
    Vector ComputePosition();

    bool ShouldCombine( const Vector& pos ) const;
    static bool ShouldBeRemoved( FireGlowEnt_t* pEnt, FireGlowType_t glowType );

    CUtlVector<FireGlowEnt_t*> vpEnts;
    FireGlowType_t glowType;
    dlight_t* pLight;

    float flNextUpdate;
    Vector vecLastPos;
    bool bDying;
};

class CZMFireGlowSystem : CAutoGameSystemPerFrame
{
public:
    CZMFireGlowSystem();
    ~CZMFireGlowSystem();

    virtual void Update( float frametime ) OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;

    int AddFireEntity( FireGlowEnt_t* pEnt );
    bool RemoveFireEntity( FireGlowEnt_t* pEnt );

    int GetMaxCount() const;

    static bool IsDebugging();

protected:
    int AttemptCombine( FireGlowEnt_t* pEnt ) const;
    int FindFireEntity( FireGlowEnt_t* pEnt ) const;
    int FindDying() const;


private:
    CUtlVectorAutoPurge<FireData_t*> m_vFireEntities;
};

extern CZMFireGlowSystem g_ZMFireGlowSystem;
