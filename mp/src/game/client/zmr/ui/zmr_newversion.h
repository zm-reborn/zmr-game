#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui/IVGui.h>

#include "zmr_int.h"


class CZMNewVerMenu : public vgui::Frame
{
public:
    DECLARE_CLASS_SIMPLE( CZMNewVerMenu, vgui::Frame );

    CZMNewVerMenu( vgui::VPANEL parent );
    ~CZMNewVerMenu();
};

extern IZMUi* g_pZMNewVerMenuInt;
extern CZMNewVerMenu* g_pZMNewVerMenu;
