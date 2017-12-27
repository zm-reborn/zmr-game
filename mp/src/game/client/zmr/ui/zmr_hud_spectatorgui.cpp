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

    m_szTargetTxt[0] = NULL;
    m_pOldTarget = nullptr;
    m_nOldObserverMode = OBS_MODE_NONE;
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
}

void CZMHudSpectatorUI::Reset()
{
    SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

void CZMHudSpectatorUI::OnThink()
{
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    if ( !pLocal || !pLocal->IsObserver() )
        return;

    if (m_pOldTarget
    &&  pLocal->GetObserverTarget() == m_pOldTarget
    &&  pLocal->GetObserverMode() == m_nOldObserverMode
    &&  m_pOldTarget->GetHealth() == m_nOldTargetHealth )
        return;

    if ( !UpdateTargetText() )
    {
        m_szTargetTxt[0] = NULL;
        return;
    }
}

void CZMHudSpectatorUI::PaintBar( int y, int h, bool bFlip )
{
    const int w = ScreenWidth();

#define SIZE_X      512.0f
#define SIZE_Y      512.0f

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
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    if ( !pLocal || !pLocal->IsObserver() )
        return;


    const int bar_height = ScreenHeight() * 0.1f;
    const int border_height = 64;

    // Top
    PaintBar( 0, bar_height, true );
    PaintBorder( bar_height, border_height, true );

    // Bottom
    PaintBar( ScreenHeight() - bar_height, bar_height, false );
    PaintBorder( ScreenHeight() - bar_height - border_height, border_height, false );
    

    DrawText( ScreenWidth() / 2.0f, ScreenHeight() - bar_height / 2.0f );
}

void CZMHudSpectatorUI::DrawText( int x, int y )
{
    if ( m_szTargetTxt[0] == NULL )
        return;


    int w, h;
    surface()->GetTextSize( m_hTextFont, m_szTargetTxt, w, h );

	surface()->DrawSetTextFont( m_hTextFont );
    surface()->DrawSetTextColor( m_TargetColor );
	surface()->DrawSetTextPos( x - w / 2.0f, y - h );
	surface()->DrawUnicodeString( m_szTargetTxt );
}

bool CZMHudSpectatorUI::UpdateTargetText()
{
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    if ( !pLocal || !pLocal->IsObserver() )
        return false;

    C_BaseEntity* pEnt = pLocal->GetObserverTarget();

    if (!pEnt
    ||  (pLocal->GetObserverMode() != OBS_MODE_IN_EYE && pLocal->GetObserverMode() != OBS_MODE_CHASE) )
    {
        return false;
    }

    const char* pszName = nullptr;

    
    C_BasePlayer* pTargetPlayer = ToBasePlayer( pEnt );
    int health = 0;

    if ( pTargetPlayer )
    {
        pszName = pTargetPlayer->GetPlayerName();
        health = pTargetPlayer->GetHealth();

        m_TargetColor = g_PR->GetTeamColor( g_PR->GetTeam( pEnt->entindex() ) );
    }
    else
    {
        C_ZMBaseZombie* pZombie = ToBaseZombie( pEnt );

        if ( pZombie )
        {
            pszName = "Zombie";

            m_TargetColor = g_PR->GetTeamColor( ZMTEAM_ZM );
        }
    }


    if ( !pszName || !*pszName )
        return false;


    

    wchar_t szName[MAX_PLAYER_NAME_LENGTH];
    g_pVGuiLocalize->ConvertANSIToUnicode( pszName, szName, ARRAYSIZE( szName ) );

    if ( health > 0 )
    {
        V_snwprintf( m_szTargetTxt, ARRAYSIZE( m_szTargetTxt ), L"%s (%i)", szName, health );
    }
    else
    {
        V_snwprintf( m_szTargetTxt, ARRAYSIZE( m_szTargetTxt ), L"%s", szName );
    }
    

    m_pOldTarget = pEnt;
    m_nOldTargetHealth = health;
    m_nOldObserverMode = pLocal->GetObserverMode();

    return true;
}
