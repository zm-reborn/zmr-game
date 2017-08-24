#include "cbase.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "hudelement.h"


#include "zmr_player_shared.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

// A bit higher than the flickering to tell the player that you're in deep shit now.
#define CRITICAL_FLASHLIGHT         20.0f


class CZMHudFlashlight : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudFlashlight, Panel );

    CZMHudFlashlight( const char *pElementName );


    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void Reset() OVERRIDE;

    virtual void OnThink() OVERRIDE;

    virtual bool ShouldDraw() OVERRIDE;
    virtual void Paint() OVERRIDE;

    void MsgFunc_Damage( bf_read &msg );

private:
    float m_flBattery;
    bool m_bIsOn;
    float m_flLastChange;


    int m_nTexOnId;
    int m_nTexOffId;


    void PaintString( HFont, const wchar_t* );
    void PaintBar();


    CPanelAnimationVar( Color, m_BgColor, "FlashlightBgColor", "64 64 64 255" );
    CPanelAnimationVar( Color, m_Color, "FlashlightColor", "ZMFgColor" );
    CPanelAnimationVar( float, m_flAlpha, "FlashlightAlpha", "255" );
    CPanelAnimationVarAliasType( float, m_flSize, "FlashlightSize", "64", "proportional_float" );
};

DECLARE_HUDELEMENT( CZMHudFlashlight );


CZMHudFlashlight::CZMHudFlashlight( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudFlashlight" )
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );


    SetPaintBackgroundEnabled( false );


    m_nTexOnId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexOnId, "zmr_effects/flashlight_on", true, false );

    m_nTexOffId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexOffId, "zmr_effects/flashlight_off", true, false );
}

void CZMHudFlashlight::Init()
{
    Reset();
}

void CZMHudFlashlight::VidInit()
{
    Reset();
}

void CZMHudFlashlight::Reset()
{
    m_flBattery = 100.0f;
    m_flLastChange = 0.0f;
}

void CZMHudFlashlight::OnThink()
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    float newbattery = pPlayer->GetFlashlightBattery();
    bool ison = pPlayer->GetEffects() & EF_DIMLIGHT ? true : false;

    if ( newbattery != m_flBattery )
    {
        if ( ison )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMFlashlightFadeIn" );


            if ( newbattery <= CRITICAL_FLASHLIGHT )
            {
                g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMFlashlightLow" );
            }
            else
            {
                g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMFlashlightNormal" );
            }
        }

        m_flLastChange = gpGlobals->curtime;
    }
    else if ( (gpGlobals->curtime - m_flLastChange) > 3.0f )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMFlashlightFadeOut" );
    }


    m_flBattery = newbattery;
    m_bIsOn = ison;
}

bool CZMHudFlashlight::ShouldDraw()
{
    if ( m_flAlpha <= 0.0f ) return false;

    return CHudElement::ShouldDraw();
}

void CZMHudFlashlight::Paint()
{
    int tex = m_bIsOn ? m_nTexOnId : m_nTexOffId;

    m_BgColor[3] = m_flAlpha;
    m_Color[3] = m_flAlpha;


    int size_x = m_flSize * 2.0f;
    int size_y = m_flSize;

    surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( tex );
    surface()->DrawTexturedRect( 0, 0, size_x, size_y );


    float battery = m_flBattery;

    if ( battery <= 0.0f ) return;
    if ( battery > 100.0f ) battery = 100.0f;


    float frac = battery / 100.0f;

    int fill_x = (int)(size_x * frac);

    surface()->DrawSetColor( m_Color );
    surface()->DrawSetTexture( tex );
    surface()->DrawTexturedSubRect( 0, 0, fill_x, size_y, 0.0f, 0.0f, frac, 1.0f );
}
