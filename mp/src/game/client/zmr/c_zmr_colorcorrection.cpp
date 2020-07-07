#include "cbase.h"


#include "c_zmr_player.h"
#include "c_zmr_colorcorrection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CZMColorCorrectionSystem* ZMGetCCSystem()
{
    static CZMColorCorrectionSystem s;
    return &s;
}


ConVar zm_cl_colorcorrection_effects( "zm_cl_colorcorrection_effects", "1", FCVAR_ARCHIVE );


//
CZMBaseCCEffect::CZMBaseCCEffect( const char* name, const char* filename )
{
    m_Hndl = INVALID_CLIENT_CCHANDLE;

    m_pszName = name;
    m_pszFilename = filename;

    ZMGetCCSystem()->AddEffect( this );
}

CZMBaseCCEffect::~CZMBaseCCEffect()
{
    ZMGetCCSystem()->RemoveEffect( this );
}

bool CZMBaseCCEffect::HasHandle() const
{
    return GetHandle() != INVALID_CLIENT_CCHANDLE;
}

const char* CZMBaseCCEffect::GetName() const
{
    return m_pszName;
}

const char* CZMBaseCCEffect::GetFilename() const
{
    return m_pszFilename;
}

ClientCCHandle_t CZMBaseCCEffect::GetHandle() const
{
    return m_Hndl;
}

void CZMBaseCCEffect::SetHandle( ClientCCHandle_t hndl )
{
    m_Hndl = hndl;
}
//


//
C_ZMEntColorCorrection::C_ZMEntColorCorrection()
{
}

C_ZMEntColorCorrection::~C_ZMEntColorCorrection()
{
    ClearEffects();

    ZMGetCCSystem()->m_pCCEnt = nullptr;
}

void C_ZMEntColorCorrection::ClientThink()
{
    if ( !zm_cl_colorcorrection_effects.GetBool() )
    {
        ClearEffects();
        return;
    }


    FOR_EACH_VEC( m_vCCs, i )
    {
        auto* eff = m_vCCs[i];

        g_pColorCorrectionMgr->SetColorCorrectionWeight( eff->GetHandle(), eff->GetWeight() );

        if ( eff->IsDone() )
        {
            g_pColorCorrectionMgr->SetColorCorrectionWeight( eff->GetHandle(), 0.0f );

            m_vCCs.Remove( i );
            --i;
        }
    }
}

void C_ZMEntColorCorrection::ClearEffects()
{
    FOR_EACH_VEC( m_vCCs, i )
    {
        auto* eff = m_vCCs[i];

        if ( eff->HasHandle() )
        {
            g_pColorCorrectionMgr->SetColorCorrectionWeight( eff->GetHandle(), 0.0f );
            
            // ZMRTODO: Figure out if this is safe to do.
            //g_pColorCorrectionMgr->RemoveColorCorrection( eff->GetHandle() );
            //eff->SetHandle( INVALID_CLIENT_CCHANDLE );
        }
    }

    //m_vCCs.RemoveAll();
}

void C_ZMEntColorCorrection::AddCC( CZMBaseCCEffect* cc )
{
    if ( m_vCCs.Find( cc ) != m_vCCs.InvalidIndex() )
        return;

    m_vCCs.AddToTail( cc );
}
//


//
CZMColorCorrectionSystem::CZMColorCorrectionSystem() : CAutoGameSystem( "ZMColorCorrectionSystem" )
{
    m_pCCEnt = nullptr;
}

CZMColorCorrectionSystem::~CZMColorCorrectionSystem()
{
    ReleaseCCEnt();
}

bool CZMColorCorrectionSystem::Init()
{
    ListenForGameEvent( "player_death" );

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

void CZMColorCorrectionSystem::AddEffect( CZMBaseCCEffect* eff )
{
    m_vEffects.AddToTail( eff );
}

bool CZMColorCorrectionSystem::RemoveEffect( CZMBaseCCEffect* eff )
{
    return m_vEffects.FindAndRemove( eff );
}

bool CZMColorCorrectionSystem::CheckCC()
{
    if ( !zm_cl_colorcorrection_effects.GetBool() )
    {
        ReleaseCCEnt();
        return false;
    }

    //if ( !IsReady() )
    //    InitCC();


    InitEnt();

    InitEffects();

    return true;
    
}

void CZMColorCorrectionSystem::InitEffects()
{
    FOR_EACH_VEC( m_vEffects, i )
    {
        auto* eff = m_vEffects[i];

        if ( !eff->HasHandle() )
        {
            eff->SetHandle( g_pColorCorrectionMgr->AddColorCorrection( eff->GetName(), eff->GetFilename() ) );
        }
    }
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
        auto* eff = m_vEffects[i];
        if ( eff->OnDeath() )
        {
            m_pCCEnt->AddCC( eff );
        }
    }
}

void CZMColorCorrectionSystem::OnTeamChange( int iTeam )
{
    if ( !CheckCC() ) return;


    FOR_EACH_VEC( m_vEffects, i )
    {
        auto* eff = m_vEffects[i];
        if ( eff->OnTeamChange( iTeam ) )
        {
            m_pCCEnt->AddCC( eff );
        }
    }
}
//
