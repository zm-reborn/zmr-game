#pragma once


#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/TextEntry.h>

#include "zmr_options_tab.h"


class CZMOptionsSubMisc : public CZMOptionsSub
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSubMisc, CZMOptionsSub );

    CZMOptionsSubMisc( vgui::Panel* parent );
    ~CZMOptionsSubMisc();

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

    MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel );
    MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

private:
    vgui::CheckButton*  m_pCheck_FpDeathcam;
    vgui::CheckButton*  m_pCheck_Deathnotice;
    vgui::CheckButton*  m_pCheck_ShowHelp;
    vgui::Slider*       m_pSlider_Fov;
    vgui::TextEntry*    m_pTextEntry_Fov;
};
