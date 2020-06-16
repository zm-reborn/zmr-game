#pragma once


typedef C_BaseEntity FireGlowEnt_t;

enum class FireGlowType_t
{
    GLOW_GENERIC_FIRE = 0,

    GLOW_MOLOTOV_FIRE,
    GLOW_IMMOLATOR
};


struct FireData_t
{
    FireGlowEnt_t* pEnt;
    FireGlowType_t glowType;
    float flNextUpdate;
};

class CZMFireGlowSystem : CAutoGameSystemPerFrame
{
public:
    CZMFireGlowSystem();
    ~CZMFireGlowSystem();

    virtual void Update( float frametime ) OVERRIDE;


    int AddFireEntity( FireGlowEnt_t* pEnt );
    bool RemoveFireEntity( FireGlowEnt_t* pEnt );

    int GetMaxCount() const;

protected:
    int FindFireEntity( FireGlowEnt_t* pEnt ) const;

private:
    CUtlVector<FireData_t> m_vFireEntities;
};

extern CZMFireGlowSystem g_ZMFireGlowSystem;
