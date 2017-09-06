#pragma once

#include <vgui_controls/AnimationController.h>
#include "hudelement.h"


class CZMHudCenterText : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudCenterText, vgui::Panel );

    CZMHudCenterText( const char* pElementName );
    ~CZMHudCenterText();


    virtual void Init( void ) OVERRIDE;
    virtual void VidInit( void ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;


    void ShowText( const char* bigtxt = nullptr, const char* smalltxt = nullptr, float smalldelay = 0.0f, float displaytime = 5.0f );
    void ShowText( const wchar_t* bigtxt = nullptr, const wchar_t* smalltxt = nullptr, float smalldelay = 0.0f, float displaytime = 5.0f );

    void MsgFunc_ZMCenterText( bf_read &msg );

private:
    void PaintString( const vgui::HFont, const vgui::HFont, const Color&, int x, int y, const wchar_t* );


    float m_flShowSmall;
    float m_flNextHide;
    wchar_t m_szBig[256];
    wchar_t m_szSmall[256];

    CPanelAnimationVar( vgui::HFont, m_hBigFont, "BigFont", "ZMHudCenterBig" );
    CPanelAnimationVar( vgui::HFont, m_hBigShadowFont, "ShadowBigFont", "ZMHudCenterBigShadow" );
    CPanelAnimationVar( vgui::HFont, m_hSmallFont, "SmallFont", "ZMHudCenterSmall" );
    CPanelAnimationVar( vgui::HFont, m_hSmallShadowFont, "ShadowSmallFont", "ZMHudCenterSmallShadow" );
    CPanelAnimationVar( Color, m_Color, "Color", "255 255 255 255" );

    CPanelAnimationVar( float, m_flBigAlpha, "BigAlpha", "0" );
    CPanelAnimationVar( float, m_flSmallAlpha, "SmallAlpha", "0" );

    CPanelAnimationVarAliasType( float, m_flBigPosY, "BigPosY", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flSmallOffsetY, "SmallOffsetY", "0", "proportional_float" );
};
