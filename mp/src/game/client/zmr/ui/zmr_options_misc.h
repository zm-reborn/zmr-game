#pragma once


#include <vgui_controls/CheckButton.h>

#include "zmr_options_tab.h"


class CZMOptionsSubMisc : public CZMOptionsSub
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsSubMisc, CZMOptionsSub );

    CZMOptionsSubMisc( vgui::Panel* parent );
    ~CZMOptionsSubMisc();

    virtual void OnApplyChanges() OVERRIDE;
    virtual void OnResetData() OVERRIDE;

private:
    vgui::CheckButton*  m_pCheck_FpDeathcam;
    vgui::CheckButton*  m_pCheck_Deathnotice;
    vgui::CheckButton*  m_pCheck_ShowHelp;
};
