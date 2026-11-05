#pragma once

#include <vgui_controls/AnimationController.h>
#include "hudelement.h"


class CZMHudBars : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudBars, vgui::Panel );

    CZMHudBars( const char *pElementName );

    virtual void Init() override;
    virtual void VidInit() override;
    virtual void LevelInit() override;
    virtual void Reset() override;

    virtual void FireGameEvent( IGameEvent* pEvent ) override;

    virtual void OnThink() override;

    virtual void Paint() override;


    static void Show( float displaytime = 0.0f );
    static void Hide();

    void ShowBars( float displaytime = 0.0f );
    void HideBars();
private:

    void PaintBorder( int border_y, int border_height, bool bFlip );

    float m_flNextHide;

    int m_nTexPanelBgEdgesId;

    CPanelAnimationVar( float, m_flAlpha, "BarAlpha", "255" );
    CPanelAnimationVar( float, m_flTopBarY, "TopBarY", "0" );
    CPanelAnimationVar( float, m_flBottomBarY, "BottomBarY", "480" );
};
