#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>

#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_hud_spectatorgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


DECLARE_HUDELEMENT( CZMHudSpectatorUI );


CZMHudSpectatorUI::CZMHudSpectatorUI( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudSpectatorUI" )
{
    SetPaintBackgroundEnabled( false );

    m_nTexPanelBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexPanelBgId, "zmr_effects/hud_bg_spec", true, false );

    m_nTexPanelBgTopId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexPanelBgTopId, "zmr_effects/hud_bg_spec_top", true, false );

    m_pOldTarget = nullptr;
    m_nOldObserverMode = OBS_MODE_NONE;


    SetProportional( true );
}

void CZMHudSpectatorUI::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    LoadControlSettings( "resource/ui/zmspectatorui.res" );


    m_pNameLabel = dynamic_cast<Label*>( FindChildByName( "ObservedTargetName" ) );
    m_pInfoLabel = dynamic_cast<Label*>( FindChildByName( "ObservedTargetInfo" ) );
}

void CZMHudSpectatorUI::Init()
{
    LevelInit();
}

void CZMHudSpectatorUI::VidInit()
{
    LevelInit();
}

void CZMHudSpectatorUI::LevelInit()
{
    Reset();

    /*
    Label* pLabel = dynamic_cast<Label*>( FindChildByName( "mapinfo" ) );

    char buffer[128];
    wchar_t wMapName[256];
    wchar_t wBuffer[256];
    Q_FileBase( engine->GetLevelName(), buffer, sizeof buffer );

    g_pVGuiLocalize->ConvertANSIToUnicode( buffer, wMapName, sizeof wMapName );
    g_pVGuiLocalize->ConstructString( wBuffer, sizeof wBuffer, g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

    pLabel->SetText( wBuffer );
    */
}

void CZMHudSpectatorUI::Reset()
{
    SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

bool CZMHudSpectatorUI::IsVisible()
{
    return BaseClass::IsVisible();
}

void CZMHudSpectatorUI::OnThink()
{
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();

    SetVisible( pLocal && pLocal->IsObserver() );



    if ( !IsVisible() ) return;


    if (m_pOldTarget
    &&  pLocal->GetObserverTarget() == m_pOldTarget
    &&  pLocal->GetObserverMode() == m_nOldObserverMode
    &&  m_pOldTarget->GetHealth() == m_nOldTargetHealth )
        return;

    Update();
}

void CZMHudSpectatorUI::PaintBar( int y, int h, bool bFlip )
{
    const int w = ScreenWidth();

#define SIZE_X      512.0f
#define SIZE_Y      256.0f

    float tx0 = 0.0f;
    float ty0 = 0.0f;
    float tx1 = (float)w / SIZE_X;
    float ty1 = (float)h / SIZE_Y;


    if ( bFlip )
    {
        tx0 = tx1; ty0 = ty1;
        tx1 = ty1 = 0.0f;
    }


    surface()->DrawSetColor( m_BarColor );
    surface()->DrawSetTexture( m_nTexPanelBgId );
    surface()->DrawTexturedSubRect( 0, y, w, y + h, tx0, ty0, tx1, ty1 );
}

void CZMHudSpectatorUI::PaintBorder( int border_y, int border_height, bool bFlip )
{
    const int w = ScreenWidth();

    float tx0 = 0.0f;
    float ty0 = 0.0f;
    float tx1 = (float)w / SIZE_X;
    float ty1 = 1.0f;


    if ( bFlip )
    {
        tx0 = tx1; ty0 = ty1;
        tx1 = ty1 = 0.0f;
    }


    surface()->DrawSetColor( m_BarColor );
    surface()->DrawSetTexture( m_nTexPanelBgTopId );
    surface()->DrawTexturedSubRect( 0, border_y, w, border_y + border_height, tx0, ty0, tx1, ty1 );
}

void CZMHudSpectatorUI::Paint()
{
    const int bar_height = ScreenHeight() * 0.1f;
    const int border_height = 64;

    // Top
    PaintBar( 0, bar_height, true );
    PaintBorder( bar_height, border_height, true );

    // Bottom
    PaintBar( ScreenHeight() - bar_height, bar_height, false );
    PaintBorder( ScreenHeight() - bar_height - border_height, border_height, false );

    BaseClass::Paint();
}

void CZMHudSpectatorUI::Update()
{
    if ( !UpdateTargetText() )
    {
        m_pNameLabel->SetText( "" );
        m_pInfoLabel->SetText( "" );
        return;
    }
}

bool CZMHudSpectatorUI::UpdateTargetText()
{
    if ( !g_PR ) return false;


    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    C_BaseEntity* pEnt = pLocal->GetObserverTarget();


    m_pOldTarget = pEnt;
    m_nOldTargetHealth = pEnt ? pEnt->GetHealth() : 0;
    m_nOldObserverMode = pLocal->GetObserverMode();


    if (!pEnt
    ||  (pLocal->GetObserverMode() != OBS_MODE_IN_EYE && pLocal->GetObserverMode() != OBS_MODE_CHASE) )
    {
        return false;
    }


    Color clr;
    const char* pszName = nullptr;
    bool bLocalization = false;

    
    C_BasePlayer* pTargetPlayer = ToBasePlayer( pEnt );
    int health = 0;

    if ( pTargetPlayer )
    {
        pszName = pTargetPlayer->GetPlayerName();
        health = pTargetPlayer->GetHealth();

        clr = g_PR->GetTeamColor( g_PR->GetTeam( pEnt->entindex() ) );
    }
    else
    {
        C_ZMBaseZombie* pZombie = dynamic_cast<C_ZMBaseZombie*>( pEnt );

        if ( pZombie )
        {
            pszName = pZombie->GetZombieLocalization();
            bLocalization = true;
        }

        clr = g_PR->GetTeamColor( ZMTEAM_ZM );
    }


    if ( !pszName || !(*pszName) )
        return false;


    wchar_t buffer[162];

    // Name label.
    if ( bLocalization )
    {
        Q_wcsncpy( buffer, g_pVGuiLocalize->Find( pszName ), sizeof( buffer ) );
    }
    else
    {
        g_pVGuiLocalize->ConvertANSIToUnicode( pszName, buffer, sizeof( buffer ) );
    }
    
    m_pNameLabel->SetText( buffer );
    m_pNameLabel->SetFgColor( clr );

    // Info label.
    if ( health > 0 )
    {
        V_snwprintf( buffer, ARRAYSIZE( buffer ), L"%i", health );
    }
    else
    {
        buffer[0] = NULL;
    }
    
    m_pInfoLabel->SetText( buffer );
    m_pInfoLabel->SetFgColor( clr );

    return true;
}
