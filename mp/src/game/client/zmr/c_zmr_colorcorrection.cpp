#include "cbase.h"


#include "c_zmr_player.h"
#include "c_zmr_colorcorrection.h"



ConVar zm_cl_colorcorrection_effects( "zm_cl_colorcorrection_effects", "1", FCVAR_ARCHIVE );


#define ZMCC_NAME_DEATH         "materials/colorcorrection/game_death.raw"


//
CZMBaseCCEffect::CZMBaseCCEffect( ClientCCHandle_t hndl )
{
    m_Hndl = hndl;


    //g_ZMColorCorrection.AddEffect( this );
}

CZMBaseCCEffect::~CZMBaseCCEffect()
{
}

ClientCCHandle_t CZMBaseCCEffect::GetHandle() const
{
    return m_Hndl;
}
//


//
C_ZMEntColorCorrection::C_ZMEntColorCorrection()
{
}

C_ZMEntColorCorrection::~C_ZMEntColorCorrection()
{
    m_vCCs.RemoveAll();

    g_ZMColorCorrection.m_pCCEnt = nullptr;
}

void C_ZMEntColorCorrection::ClientThink()
{
    FOR_EACH_VEC( m_vCCs, i )
    {
        CZMBaseCCEffect* eff = m_vCCs[i];

        g_pColorCorrectionMgr->SetColorCorrectionWeight( eff->GetHandle(), eff->GetWeight() );

        if ( eff->IsDone() )
        {
            m_vCCs.Remove( i );
            --i;
        }
    }
}

void C_ZMEntColorCorrection::AddCC( CZMBaseCCEffect* cc )
{
    m_vCCs.AddToTail( cc );
}
//


//
CZMColorCorrectionSystem::CZMColorCorrectionSystem()
{
    m_pCCEnt = nullptr;
}

CZMColorCorrectionSystem::~CZMColorCorrectionSystem()
{
    ReleaseCCEnt();
}

bool CZMColorCorrectionSystem::Init()
{
    //ListenForGameEvent( "player_death" );

    return true;
}

void CZMColorCorrectionSystem::FireGameEvent( IGameEvent* event )
{
    int index = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

    C_ZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( index ) );
    if ( pPlayer && pPlayer->IsLocalPlayer() )
    {
        OnDeath();
    }
}

bool CZMColorCorrectionSystem::IsReady() const
{
    return true;//m_CC_Death != INVALID_CLIENT_CCHANDLE;
}

bool CZMColorCorrectionSystem::CheckCC()
{
    return false;
    /*
    if ( !zm_cl_colorcorrection_effects.GetBool() )
        return false;


    if ( !IsReady() )
        InitCC();


    InitEnt();

    return true;
    */
}

void CZMColorCorrectionSystem::InitCC()
{
    //m_CC_Death = g_pColorCorrectionMgr->AddColorCorrection( ZMCC_NAME_DEATH );

}

void CZMColorCorrectionSystem::InitEnt()
{
    if ( !m_pCCEnt )
    {
        m_pCCEnt = new C_ZMEntColorCorrection;
        m_pCCEnt->index = -1;

        cl_entitylist->AddNonNetworkableEntity( m_pCCEnt->GetIClientUnknown() );
        m_pCCEnt->SetNextClientThink( CLIENT_THINK_ALWAYS );
    }
}

void CZMColorCorrectionSystem::ReleaseCCEnt()
{
    delete m_pCCEnt;
    m_pCCEnt = nullptr;
}

void CZMColorCorrectionSystem::OnDeath()
{
    if ( !CheckCC() ) return;


    FOR_EACH_VEC( m_vEffects, i )
    {
        m_pCCEnt->AddCC( m_vEffects[i] );
    }
    
}

void CZMColorCorrectionSystem::OnTeamChange( int iTeam )
{
    CheckCC();
}
//


CZMColorCorrectionSystem g_ZMColorCorrection;




class CZMDeathEffect : public CZMBaseCCEffect
{
public:
    CZMDeathEffect( ClientCCHandle_t hndl ) : CZMBaseCCEffect( hndl )
    {
        m_flEnd = gpGlobals->curtime + 6.0f;
    }

    virtual float GetWeight() OVERRIDE
    {
        float v = m_flEnd - gpGlobals->curtime;
        return clamp( v / 2.5f, 0.0f, 1.0f );
    }

    virtual bool IsDone() OVERRIDE
    {
        return (m_flEnd - gpGlobals->curtime) <= 0.0f;
    }

private:
    float m_flEnd;
};

//CZMDeathEffect g_ZMCCDeathEffect;
