#include "cbase.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "zmr_hud_centertext.h"



using namespace vgui;

DECLARE_HUDELEMENT( CZMHudCenterText );
DECLARE_HUD_MESSAGE( CZMHudCenterText, ZMCenterText );


CZMHudCenterText::CZMHudCenterText( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudCenterText" )
{
    SetPaintBackgroundEnabled( false );


    Reset();
}

CZMHudCenterText::~CZMHudCenterText()
{
    
}

void CZMHudCenterText::Init( void )
{
    HOOK_HUD_MESSAGE( CZMHudCenterText, ZMCenterText );

    Reset();
}

void CZMHudCenterText::VidInit( void )
{
    Reset();

    SetSize( ScreenWidth(), ScreenHeight() );
}

void CZMHudCenterText::Reset()
{
    m_szBig[0] = NULL;
    m_szSmall[0] = NULL;
    m_flShowSmall = 0.0f;
    m_flNextHide = 0.0f;

    Msg( "Reset()\n" );
}

void CZMHudCenterText::OnThink()
{
    if ( m_flNextHide != 0.0f )
    {
        if ( m_flNextHide <= gpGlobals->curtime )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeOut" );

            m_flNextHide = 0.0f;
        }

        if ( m_flShowSmall != 0.0f && m_flShowSmall <= gpGlobals->curtime )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeInSmall" );

            m_flShowSmall = 0.0f;
        }
    }
}

void CZMHudCenterText::PaintString( const HFont font, const HFont shadowfont, const Color& clr, int x, int y, const wchar_t* txt )
{
	surface()->DrawSetTextPos( x, y );
    surface()->DrawSetTextColor( Color( 0, 0, 0, clr[3] ) );
    surface()->DrawSetTextFont( shadowfont );
	surface()->DrawUnicodeString( txt );

    surface()->DrawSetTextPos( x, y );
    surface()->DrawSetTextColor( clr );
    surface()->DrawSetTextFont( font );
	surface()->DrawUnicodeString( txt );
}

void CZMHudCenterText::Paint()
{
    if ( m_flBigAlpha <= 0.0f && m_flSmallAlpha <= 0.0f ) return;


    int h_big;
    int w, h;
    surface()->GetTextSize( m_hBigFont, m_szBig, w, h_big );

    if ( m_szBig[0] != NULL )
    {
        m_Color[3] = m_flBigAlpha;
        PaintString( m_hBigFont, m_hBigShadowFont, m_Color, ScreenWidth() / 2.0f - w / 2.0f, m_flBigPosY, m_szBig );
    }

    if ( m_szSmall[0] != NULL )
    {
        surface()->GetTextSize( m_hSmallFont, m_szSmall, w, h );

        m_Color[3] = m_flSmallAlpha;
        PaintString( m_hSmallFont, m_hSmallShadowFont, m_Color, ScreenWidth() / 2.0f - w / 2.0f, m_flBigPosY + h_big + m_flSmallOffsetY, m_szSmall );
    }
}

void CZMHudCenterText::ShowText( const wchar_t* bigtxt, const wchar_t* smalltxt, float smalldelay, float displaytime )
{
    if ( bigtxt != nullptr )
    {
        Q_wcsncpy( m_szBig, bigtxt, sizeof( m_szBig ) );
    }

    if ( smalltxt != nullptr )
    {
        Q_wcsncpy( m_szSmall, smalltxt, sizeof( m_szSmall ) );
    }

    if ( bigtxt || smalltxt )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeIn" );

        m_flNextHide = gpGlobals->curtime + displaytime;


        m_flShowSmall = gpGlobals->curtime + smalldelay;
    }
    else
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeOut" );
    }
}

void CZMHudCenterText::MsgFunc_ZMCenterText( bf_read& msg )
{
    char buffer[256];
    wchar_t big[256];
    wchar_t small[256];


    msg.ReadString( buffer, sizeof( buffer ), true );
    g_pVGuiLocalize->ConvertANSIToUnicode( buffer, big, sizeof( big ) );

    msg.ReadString( buffer, sizeof( buffer ), true );
    g_pVGuiLocalize->ConvertANSIToUnicode( buffer, small, sizeof( small ) );


    ShowText( big, small );
}

