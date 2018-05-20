#pragma once


#include <vgui_controls/CheckButton.h>
//#include <vgui_controls/Slider.h>
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


    MESSAGE_FUNC_PARAMS( OnComboChanged, "TextChanged", data );

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

    void UpdateCrosshair( int index );
    void UpdateSettings( int index );


    vgui::ComboBox*     m_pCrosshairCombo;
    CZMPanelCrosshair*  m_pCrosshairPanel;
    vgui::TextEntry*    m_pColorRed;
    vgui::TextEntry*    m_pColorGreen;
    vgui::TextEntry*    m_pColorBlue;
    vgui::TextEntry*    m_pColorAlpha;
    vgui::TextEntry*    m_pOutlineColorRed;
    vgui::TextEntry*    m_pOutlineColorGreen;
    vgui::TextEntry*    m_pOutlineColorBlue;
    vgui::TextEntry*    m_pOutlineColorAlpha;
    vgui::TextEntry*    m_pOutlineSize;
    vgui::TextEntry*    m_pCrossType;
    vgui::TextEntry*    m_pDynMove;
    vgui::TextEntry*    m_pOffsetFromCenter;


    CUtlVector<CZMBaseCrosshair*> m_vCrosshairs;
};
