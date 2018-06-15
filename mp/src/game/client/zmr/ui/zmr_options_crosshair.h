#pragma once


#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>


#include "zmr_panel_crosshair.h"
#include "zmr_options_tab.h"


class CZMOptionsSubCrosshair : public CZMOptionsSub
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSubCrosshair, CZMOptionsSub );

    CZMOptionsSubCrosshair( vgui::Panel* parent );
    ~CZMOptionsSubCrosshair();


    MESSAGE_FUNC_PTR( OnComboChanged, "TextChanged", panel );
    MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel );
    //MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

    void UpdateCrosshair( int index = -1 );
    void UpdateSettings( int index = -1 );


    vgui::ComboBox*     m_pCrosshairCombo;
    CZMPanelCrosshair*  m_pCrosshairPanel;
    vgui::Slider*       m_pOutlineSize;
    vgui::Slider*       m_pDotSize;
    vgui::Slider*       m_pDynMove;
    vgui::Slider*       m_pOffsetFromCenter;
    vgui::Slider*       m_pSlider_ColorR;
    vgui::Slider*       m_pSlider_ColorG;
    vgui::Slider*       m_pSlider_ColorB;
    vgui::Slider*       m_pSlider_ColorA;


    CUtlVector<CZMBaseCrosshair*> m_vCrosshairs;

    int m_iCurCrosshair;
};
