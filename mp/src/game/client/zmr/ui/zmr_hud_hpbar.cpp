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


#define CRITICAL_HEALTH         25


class CZMHudHPBar : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudHPBar, Panel );

    CZMHudHPBar( const char *pElementName );


    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;

    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;

    void MsgFunc_Damage( bf_read &msg );

private:
    int m_nHealth;

    int m_nTexBgId;
    int m_nTexFgId;


    void PaintString( HFont, int, int, const Color&, const wchar_t* );
    void PaintBar();


    CPanelAnimationVarAliasType( float, m_flBarX, "BarX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flBarY, "BarY", "0", "proportional_float" );

    CPanelAnimationVarAliasType( float, m_flBarSize, "BarSize", "64", "proportional_float" );
    CPanelAnimationVar( float, m_flBlur, "Blur", "0" );

    CPanelAnimationVar( HFont, m_hFont, "HealthBarFont", "HudNumbers" );
    CPanelAnimationVar( HFont, m_hGlowFont, "HealthBarGlowFont", "HudNumbersGlow" );

    CPanelAnimationVarAliasType( float, m_flHealthX, "HealthX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flHealthY, "HealthY", "0", "proportional_float" );
};

DECLARE_HUDELEMENT( CZMHudHPBar );
DECLARE_HUD_MESSAGE( CZMHudHPBar, Damage );


CZMHudHPBar::CZMHudHPBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudHPBar" )
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );


    SetPaintBackgroundEnabled( false );


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hpbar_bg", true, false );

    m_nTexFgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexFgId, "zmr_effects/hpbar_fg", true, false );
}

void CZMHudHPBar::Init()
{
    HOOK_HUD_MESSAGE( CZMHudHPBar, Damage );

    m_nHealth = -1;
}

void CZMHudHPBar::VidInit()
{
    m_nHealth = -1;
}

void CZMHudHPBar::OnThink()
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    int newhp = pPlayer->GetHealth();


    if ( newhp != m_nHealth )
    {
        if ( newhp < CRITICAL_HEALTH )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMHealthLow" );
        }
        else
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMHealthNormal" );
        }
    }

    m_nHealth = newhp;
}

void CZMHudHPBar::Paint()
{
    PaintBar();


    int hp = m_nHealth;
    if ( hp < 0 ) hp = 0;


    wchar_t szHp[8];
    V_snwprintf( szHp, ARRAYSIZE( szHp ), L"%i", hp );


    Color clr = GetFgColor();

    surface()->DrawSetTextColor( clr );
    surface()->DrawSetTextPos( m_flHealthX, m_flHealthY );
    surface()->DrawSetTextFont( m_hFont );
    surface()->DrawUnicodeString( szHp );


    // Draw glow.
    
    for ( float fl = m_flBlur; fl > 0.0f; fl -= 1.0f )
    {
        if (fl >= 1.0f)
        {
            PaintString( m_hGlowFont, m_flHealthX, m_flHealthY, clr, szHp );
        }
        else
        {
            Color col = clr;
            col[3] *= fl;
            
            PaintString( m_hGlowFont, m_flHealthX, m_flHealthY, col, szHp );
        }
    }
}

void CZMHudHPBar::PaintString( HFont font, int x, int y, const Color& clr, const wchar_t* txt )
{
    surface()->DrawSetTextColor( clr );
    surface()->DrawSetTextPos( m_flHealthX, m_flHealthY );
    surface()->DrawSetTextFont( m_hGlowFont );
    surface()->DrawUnicodeString( txt );
}

void CZMHudHPBar::PaintBar()
{
    surface()->DrawSetColor( COLOR_WHITE );

    int size_x = m_flBarSize;
    int size_y = m_flBarSize;

    int offset_x = m_flBarX;
    int offset_y = m_flBarY;

    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( offset_x, offset_y, offset_x + size_x, offset_y + size_y );


    float hp = (float)m_nHealth;

    if ( hp <= 0.0f ) return;
    if ( hp > 100.0f ) hp = 100.0f;


    float frac = 1.0f - (hp / 100.0f);

    int fill_y = (int)(m_flBarSize * frac);

    surface()->DrawSetTexture( m_nTexFgId );
    surface()->DrawTexturedSubRect( offset_x, offset_y + fill_y, offset_x + size_x, offset_y + size_y, 0.0f, frac, 1.0f, 1.0f );
}

void CZMHudHPBar::MsgFunc_Damage( bf_read &msg )
{
    msg.ReadByte(); // armor
    int dmg = msg.ReadByte();
    msg.ReadLong(); // dmgtype

    // Origin
    msg.ReadBitCoord();
    msg.ReadBitCoord();
    msg.ReadBitCoord();


    if ( dmg > 0 )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMHealthDamaged" );
    }
}