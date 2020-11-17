#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>


#include "zmr_hud_blackbars.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


extern ConVar zm_sv_roundintermissiontime;
extern ConVar zm_cl_round_cinematic_effects_duration;

ConVar zm_cl_round_cinematic_effects( "zm_cl_round_cinematic_effects", "1", FCVAR_ARCHIVE, "The cinematic effects (black bars)" );


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

    ListenForGameEvent( "round_end_post" );
    ListenForGameEvent( "round_restart_post" );

    m_nTexPanelBgEdgesId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexPanelBgEdgesId, "zmr_effects/hud_blackbars_top", true, false );
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

void CZMHudBars::FireGameEvent( IGameEvent* pEvent )
{
    //
    // Round start/end effects
    //
    if ( Q_strcmp( pEvent->GetName(), "round_end_post" ) == 0 )
    {
        if ( zm_cl_round_cinematic_effects.GetBool() )
        {
            // Add a bit of padding in case of lag.
            ShowBars( zm_sv_roundintermissiontime.GetFloat() + 0.5f );
        }
    }
    else if ( Q_strcmp( pEvent->GetName(), "round_restart_post" ) == 0 )
    {
        if ( zm_cl_round_cinematic_effects.GetBool() )
        {
            ShowBars( zm_cl_round_cinematic_effects_duration.GetFloat() );
        }
    }
}

void CZMHudBars::OnThink()
{
    if ( m_flNextHide != 0.0f && m_flNextHide <= gpGlobals->curtime )
    {
        HideBars();
    }
}

void CZMHudBars::PaintBorder( int border_y, int border_height, bool bFlip )
{
    const int w = ScreenWidth();

    float tx0 = 0.0f;
    float ty0 = 0.0f;
    float tx1 = (float)w / 512.0f;
    float ty1 = 1.0f;


    if ( bFlip )
    {
        tx0 = tx1; ty0 = ty1;
        tx1 = ty1 = 0.0f;
    }


    surface()->DrawSetTexture( m_nTexPanelBgEdgesId );
    surface()->DrawTexturedSubRect( 0, border_y, w, border_y + border_height, tx0, ty0, tx1, ty1 );
}

void CZMHudBars::Paint()
{
    if ( m_flAlpha <= 0.0f ) return;
    

    Color clr = Color( 0, 0, 0, m_flAlpha );

    surface()->DrawSetColor( clr );


    const int w = ScreenWidth();
    const int h = GetTall();


    const int border_height = 64;


    //
    // Top bar
    //
    if ( m_flTopBarY > 0.0f )
    {
        int y = YRES( m_flTopBarY ) - border_height;
        surface()->DrawFilledRect( 0, 0, w, y );

        PaintBorder( y, border_height, true );
    }

    //
    // Bottom bar
    //
    if ( m_flBottomBarY < 480.0f )
    {
        int y = YRES( m_flBottomBarY ) + border_height;
        surface()->DrawFilledRect( 0, y, w, h );

        PaintBorder( y - border_height, border_height, false );
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
