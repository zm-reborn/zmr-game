#include "cbase.h"

#include "c_zmr_colorcorrection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ZMCC_DEATH              "materials/colorcorrection/game_death.raw"


class CZMDeathEffect : public CZMBaseCCEffect
{
public:
    CZMDeathEffect() : CZMBaseCCEffect( "zmgame_death", ZMCC_DEATH )
    {
        m_flEnd = 0.0f;
    }

    virtual bool OnDeath() OVERRIDE
    {
        m_flEnd = gpGlobals->curtime + 6.0f;

        return true;
    }

    virtual float GetWeight() const OVERRIDE
    {
        float v = m_flEnd - gpGlobals->curtime;
        return clamp( v / 2.5f, 0.0f, 1.0f );
    }

    virtual bool IsDone() const OVERRIDE
    {
        return (m_flEnd - gpGlobals->curtime) <= 0.0f;
    }

private:
    float m_flEnd;
};

CZMDeathEffect g_ZMCCDeathEffect;

