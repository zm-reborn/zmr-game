#pragma once

#include "zmr_player_shared.h"

class CZMResourceSystem
{
public:
    CZMResourceSystem();
    ~CZMResourceSystem();


    void OnRoundStart();
    void GainResources( CZMPlayer* pPlayer );

    int GetResourceLimit() const;
    float GetResourcesPerMinuteMin() const;
    float GetResourcesPerMinuteMax() const;
    float GetResourcesPerMinute() const;
    float GetResourcesPerMinute( float frac ) const;

    bool UpdateState();

protected:
    int GetSurvivorCount() const;
    int GetRealSurvivorCount() const;
    int GetMaxSurvivors() const;


    void ResetState();

private:
    int m_nEffectiveSurvivorCount;
    int m_nAliveSurvivors;
    int m_nMaxSurvivors;

    int m_iLastUpdateTick;
    float m_flPlayerResourceDecimals[MAX_PLAYERS];
};

extern CZMResourceSystem g_ZMResourceSystem;
