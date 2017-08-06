#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include "hudelement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


class CZMHudHPBar : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudHPBar, Panel );

    CZMHudHPBar( const char *pElementName );


    virtual void Paint() OVERRIDE;

private:
    int m_nTexBgId;
    int m_nTexFgId;


    void PaintBar();


    CPanelAnimationVarAliasType( float, m_flBarX, "BarX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flBarY, "BarY", "0", "proportional_float" );

    CPanelAnimationVarAliasType( float, m_flBarSize, "BarSize", "64", "proportional_float" );


    CPanelAnimationVar( HFont, m_hFont, "HealthBarFont", "HudNumbers" );

    CPanelAnimationVarAliasType( float, m_flHealthX, "HealthX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flHealthY, "HealthY", "0", "proportional_float" );
};

DECLARE_HUDELEMENT( CZMHudHPBar );


CZMHudHPBar::CZMHudHPBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudHPBar" )
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );


    SetPaintBackgroundEnabled( false );


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hpbar_bg", true, false );

    m_nTexFgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexFgId, "zmr_effects/hpbar_fg", true, false );
}

void CZMHudHPBar::PaintBar()
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    surface()->DrawSetColor( COLOR_WHITE );

    int size_x = m_flBarSize;
    int size_y = m_flBarSize;

    int offset_x = m_flBarX;
    int offset_y = m_flBarY;

    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( offset_x, offset_y, offset_x + size_x, offset_y + size_y );


    float hp = (float)pPlayer->GetHealth();

    if ( hp <= 0.0f ) return;
    if ( hp > 100.0f ) hp = 100.0f;


    float frac = 1.0f - (hp / 100.0f);

    int fill_y = (int)(m_flBarSize * frac);

    surface()->DrawSetTexture( m_nTexFgId );
    surface()->DrawTexturedSubRect( offset_x, offset_y + fill_y, offset_x + size_x, offset_y + size_y, 0.0f, frac, 1.0f, 1.0f );
}

void CZMHudHPBar::Paint()
{
    PaintBar();


    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    int hp = pPlayer->GetHealth();
    if ( hp < 0 ) hp = 0;


    wchar_t szHp[8];
    V_snwprintf( szHp, ARRAYSIZE( szHp ), L"%i", hp );


    surface()->DrawSetTextColor( GetFgColor() );
    surface()->DrawSetTextFont( m_hFont );
    surface()->DrawSetTextPos( m_flHealthX, m_flHealthY );

    surface()->DrawUnicodeString( szHp );
}