#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>


//#include "zmr_hud_bars.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


#define SHOW_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsShow" )
#define HIDE_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsHide" )


CON_COMMAND( zm_hudbars_show, "" )
{
    SHOW_BARS;
}

CON_COMMAND( zm_hudbars_hide, "" )
{
    HIDE_BARS;
}


class CZMHudBars : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudBars, Panel );

    CZMHudBars( const char *pElementName );


    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void LevelInit() OVERRIDE;

    virtual void Paint() OVERRIDE;
private:

    CPanelAnimationVar( float, m_flAlpha, "BarAlpha", "255" );
    CPanelAnimationVar( float, m_flTopBarY, "TopBarY", "0" );
    CPanelAnimationVar( float, m_flBottomBarY, "BottomBarY", "480" );
};

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
    HIDE_BARS;
}

void CZMHudBars::Paint()
{
    if ( m_flAlpha <= 0.0f ) return;
    

    Color clr = Color( 0, 0, 0, m_flAlpha );

    surface()->DrawSetColor( clr );

    if ( m_flTopBarY > 0.0f )
    {
        surface()->DrawFilledRect( 0, 0, ScreenWidth(), YRES( m_flTopBarY ) );
    }

    if ( m_flBottomBarY < 480.0f )
    {
        surface()->DrawFilledRect( 0, YRES( m_flBottomBarY ), ScreenWidth(), ScreenHeight() );
    }
}
