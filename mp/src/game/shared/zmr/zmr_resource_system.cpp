#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_playerresource.h"
#endif

#include "zmr_resource_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef GAME_DLL
// Unused
ConVar zm_sv_resource_rate( "zm_sv_resource_rate", "5" );
#endif

ConVar zm_sv_resource_refill_min( "zm_sv_resource_refill_min", "420", FCVAR_NOTIFY | FCVAR_REPLICATED, "Minimum amount of resources per minute." );
ConVar zm_sv_resource_refill_max( "zm_sv_resource_refill_max", "1200", FCVAR_NOTIFY | FCVAR_REPLICATED, "Maximum amount of resources per minute." );
ConVar zm_sv_resource_refill_usehighest( "zm_sv_resource_refill_usehighest", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Is refilling based on current human count or count at the start of the round." );
ConVar zm_sv_resource_max( "zm_sv_resource_max", "5000", FCVAR_NOTIFY | FCVAR_REPLICATED );

CZMResourceSystem::CZMResourceSystem()
{
    m_nEffectiveSurvivorCount = 0;
    m_nAliveSurvivors = 0;
    m_nMaxSurvivors = 0;

    ResetState();
}

CZMResourceSystem::~CZMResourceSystem()
{
}

void CZMResourceSystem::ResetState()
{
    for ( int i = 0; i < MAX_PLAYERS; i++ )
    {
        m_flPlayerResourceDecimals[i] = 0.0f;
    }

    m_nEffectiveSurvivorCount = 0;

    m_iLastUpdateTick = -1;
}

void CZMResourceSystem::OnRoundStart()
{
    ResetState();

    UpdateState();
}

void CZMResourceSystem::GainResources( CZMPlayer* pPlayer )
{
    int playerIndex = pPlayer->entindex();
    if ( playerIndex < 0 || playerIndex >= MAX_PLAYERS )
    {
        return;
    }


    float delta = gpGlobals->frametime;


    UpdateState();


    if ( pPlayer->GetResources() >= GetResourceLimit() )
    {
        return;
    }

    
    float rpm = GetResourcesPerMinute();
    auto rps = rpm / 60.0f;
    
    float decimals = m_flPlayerResourceDecimals[playerIndex];
    decimals += delta * rps;

    float flr = floorf( decimals );
    if ( flr != 0.0f )
    {
        pPlayer->IncResources( (int)flr, false );

        decimals -= flr;
    }

    m_flPlayerResourceDecimals[playerIndex] = decimals;
}

bool CZMResourceSystem::UpdateState()
{
    if ( gpGlobals->framecount == m_iLastUpdateTick )
    {
        return false;
    }


    int nLastEffectiveCount = m_nEffectiveSurvivorCount;

    m_nAliveSurvivors = 0;
    m_nEffectiveSurvivorCount = 0;
    m_nMaxSurvivors = gpGlobals->maxClients;


#ifdef GAME_DLL
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        auto* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );
        if ( !pPlayer ) continue;


        if ( pPlayer->IsHuman() && pPlayer->IsAlive() )
            m_nAliveSurvivors++;

        if ( pPlayer->IsZM() )
            m_nMaxSurvivors--;
    }
#else
    if ( g_PR )
    {
        for ( int i = 0; i < MAX_PLAYERS; i++ )
        {
            if ( !g_PR->IsConnected( i ) ) continue;


            int iTeam = g_PR->GetTeam( i );


            if ( iTeam == ZMTEAM_HUMAN && g_PR->IsAlive( i ) )
                m_nAliveSurvivors++;

            if ( iTeam == ZMTEAM_ZM )
                m_nMaxSurvivors--;
        }
    }
#endif


    if ( zm_sv_resource_refill_usehighest.GetBool() )
    {
        m_nEffectiveSurvivorCount = MAX( nLastEffectiveCount, m_nAliveSurvivors );
    }
    else
    {
        m_nEffectiveSurvivorCount = m_nAliveSurvivors;
    }



    m_iLastUpdateTick = gpGlobals->framecount;

    return true;
}

int CZMResourceSystem::GetResourceLimit() const
{
    return zm_sv_resource_max.GetInt();
}

int CZMResourceSystem::GetSurvivorCount() const
{
    return m_nEffectiveSurvivorCount;
}

int CZMResourceSystem::GetRealSurvivorCount() const
{
    return m_nAliveSurvivors;
}

int CZMResourceSystem::GetMaxSurvivors() const
{
    return m_nMaxSurvivors;
}

float CZMResourceSystem::GetResourcesPerMinuteMin() const
{
    return zm_sv_resource_refill_min.GetFloat();
}

float CZMResourceSystem::GetResourcesPerMinuteMax() const
{
    return zm_sv_resource_refill_max.GetFloat();
}

float CZMResourceSystem::GetResourcesPerMinute() const
{
    int nSurvivors = GetSurvivorCount() - 1;
    nSurvivors = MAX( nSurvivors, 0 );
    
    float frac = nSurvivors / (float)(GetMaxSurvivors() - 1);

    return GetResourcesPerMinute( frac );
}

float CZMResourceSystem::GetResourcesPerMinute( float frac ) const
{
    float min = GetResourcesPerMinuteMin();
    float max = GetResourcesPerMinuteMax();

    return min + SimpleSpline( frac ) * (max - min);
}

CZMResourceSystem g_ZMResourceSystem;
