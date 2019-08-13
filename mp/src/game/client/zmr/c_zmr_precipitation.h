#pragma once


enum PrecipitationQuality_t
{
    PRECIPQ_NONE = 0,

    PRECIPQ_LOW,
    PRECIPQ_MEDIUM,
    PRECIPQ_HIGH
};

class C_ZMEntPrecipitation;
class C_BasePlayer;

class C_ZMPrecipitationSystem : public CAutoGameSystemPerFrame
{
public:
    C_ZMPrecipitationSystem();
    ~C_ZMPrecipitationSystem();


    virtual void PostInit() OVERRIDE;
    virtual void Update( float frametime ) OVERRIDE;

    bool AddPrecipitation( C_ZMEntPrecipitation* pEnt );
    bool RemovePrecipitation( C_ZMEntPrecipitation* pEnt );

    float GetCurrentDensity() const;
    PrecipitationQuality_t GetQuality() const;

    CParticleProperty* ParticleProp();

protected:
    void InitializeParticles();
    void BuildRayTracingEnv();

    void UpdateParticles();

    void DestroyInnerParticlePrecip();
    void DestroyOuterParticlePrecip();

    void DispatchOuterParticlePrecip( C_BasePlayer* pPlayer, const Vector& vForward );
    void DispatchInnerParticlePrecip( C_BasePlayer* pPlayer, const Vector& vForward );


    CUtlVector<C_ZMEntPrecipitation*> m_vPrecipitations;


    TimedEvent m_tParticlePrecipTraceTimer;

    HPARTICLEFFECT m_pParticlePrecipInner;
    HPARTICLEFFECT m_pParticlePrecipOuter;


    const char* m_pszParticleInner;
    const char* m_pszParticleOuter;

    bool m_bInitialized;

    PrecipitationQuality_t m_iLastQuality;
};

extern C_ZMPrecipitationSystem* ZMGetPrecipitationSystem();
