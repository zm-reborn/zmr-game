#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>


#include "zmr_hud_blackbars.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


#define SHOW_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsShow" )
#define HIDE_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsHide" )


CON_COMMAND( zm_hudbars_show, "" )
{
    CZMHudBars::Show( 9999.0f );
}

CON_COMMAND( zm_hudbars_hide, "" )
{
    CZMHudBars::Hide();
}


DECLARE_HUDELEMENT( CZMHudBars );


CZMHudBars::CZMHudBars( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudBars" )
{
    SetPaintBackgroundEnabled( false );
    SetZPos( 9000 );
}

void CZMHudBars::Init()
{
    LevelInit();
}

void CZMHudBars::VidInit()
{
    LevelInit();
}

void CZMHudBars::LevelInit()
{
    HideBars();

    Reset();
}

void CZMHudBars::Reset()
{
    SetWide( ScreenWidth() );
}

void CZMHudBars::OnThink()
{
    if ( m_flNextHide != 0.0f && m_flNextHide <= gpGlobals->curtime )
    {
        HideBars();
    }
}

void CZMHudBars::Paint()
{
    if ( m_flAlpha <= 0.0f ) return;
    

    Color clr = Color( 0, 0, 0, m_flAlpha );

    surface()->DrawSetColor( clr );

    int w = GetWide();
    int h = GetTall();

    if ( m_flTopBarY > 0.0f )
    {
        surface()->DrawFilledRect( 0, 0, w, YRES( m_flTopBarY ) );
    }

    if ( m_flBottomBarY < 480.0f )
    {
        surface()->DrawFilledRect( 0, YRES( m_flBottomBarY ), w, h );
    }
}

void CZMHudBars::Show( float displaytime )
{
    CZMHudBars* blackbars = GET_HUDELEMENT( CZMHudBars );
    if ( blackbars )
    {
        blackbars->ShowBars( displaytime );
    }
}

void CZMHudBars::Hide()
{
    CZMHudBars* blackbars = GET_HUDELEMENT( CZMHudBars );
    if ( blackbars )
    {
        blackbars->HideBars();
    }
}

void CZMHudBars::ShowBars( float displaytime )
{
    SHOW_BARS;

    if ( displaytime > 0.0f )
        m_flNextHide = gpGlobals->curtime + displaytime;
    else
        m_flNextHide = 0.0f;
}

void CZMHudBars::HideBars()
{
    HIDE_BARS;
    m_flNextHide = 0.0f;
}
