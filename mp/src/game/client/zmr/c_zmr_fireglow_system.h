#pragma once

#include "dlight.h"

typedef C_BaseEntity FireGlowEnt_t;

enum class FireGlowType_t
{
    GLOW_GENERIC_FIRE = 0, // Just a generic env_fire

    GLOW_ENTITY_FLAME, // An entity (prop/character) is on fire.
};


struct FireGlow_t
{
    FireGlow_t( FireGlowEnt_t* pEnt, FireGlowType_t glowType );
    ~FireGlow_t();

    void Update( float flUpdateInterval );
    void Kill();
    void Decay( float tim );
    bool DecaySingleEntity( float tim, FireGlowEnt_t* pEnt );
    Vector ComputePosition();

    bool ShouldCombine( const Vector& pos ) const;
    static bool IsWorldLightSufficient( FireGlowEnt_t* pEnt, FireGlowType_t glowType );

    CUtlVector<FireGlowEnt_t*> vpEnts;
    FireGlowType_t glowType;
    dlight_t* pLight;

    float flNextUpdate;
    Vector vecLastPos;
    bool bDying;
    float flNextFlicker;
    bool bCheckPositionLightLevel;
};

class CZMFireGlowSystem : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
    CZMFireGlowSystem();
    ~CZMFireGlowSystem();

    virtual void PostInit() OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;
    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;
    virtual void Update( float frametime ) OVERRIDE;
    

    int AddFireEntity( FireGlowEnt_t* pEnt, FireGlowType_t glowType );
    bool RemoveFireEntity( FireGlowEnt_t* pEnt );

    int GetMaxCount() const;

    static bool IsDebugging();
    static bool WaitMapStart();

protected:
    int AttemptCombine( FireGlowEnt_t* pEnt, FireGlowType_t glowType ) const;
    int FindFireEntity( FireGlowEnt_t* pEnt ) const;
    int FindDying() const;
    int FindSuitableReplacement() const;


private:
    CUtlVectorAutoPurge<FireGlow_t*> m_vFireEntities;

    static CountdownTimer m_MapStartTimer;
};

extern CZMFireGlowSystem g_ZMFireGlowSystem;
