#pragma once


#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>

#include "zmr_options_tab.h"
#include "zmr/ui/zmr_panel_modelpanel.h"


class CZMOptionsSubGeneral : public CZMOptionsSub
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSubGeneral, CZMOptionsSub );

    CZMOptionsSubGeneral( vgui::Panel* parent );
    ~CZMOptionsSubGeneral();

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

    MESSAGE_FUNC_PARAMS( OnComboChanged, "TextChanged", data );

private:
    CZMModelPanel*      m_pModelPanel;
    vgui::ComboBox*     m_pModelCombo;
    vgui::ComboBox*     m_pPartBox;
    vgui::CheckButton*  m_pCheck_NewMenus;
    vgui::CheckButton*  m_pCheck_PowerUser;
    vgui::CheckButton*  m_pCheck_BoxPowerUser;
    vgui::Slider*       m_pSlider_Pitch;
    vgui::Slider*       m_pSlider_Yaw;

    const char* GetCurrentPlayerModel();
    void AppendCustomModels();
};
