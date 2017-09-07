#pragma once

#include <vgui_controls/AnimationController.h>
#include "hudelement.h"


class CZMHudBars : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudBars, vgui::Panel );

    CZMHudBars( const char *pElementName );


    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void LevelInit() OVERRIDE;
    virtual void Reset() OVERRIDE;

    virtual void OnThink() OVERRIDE;

    virtual void Paint() OVERRIDE;


    void ShowBars( float displaytime = 0.0f );
    void HideBars();
private:


    float m_flNextHide;


    CPanelAnimationVar( float, m_flAlpha, "BarAlpha", "255" );
    CPanelAnimationVar( float, m_flTopBarY, "TopBarY", "0" );
    CPanelAnimationVar( float, m_flBottomBarY, "BottomBarY", "480" );
};
