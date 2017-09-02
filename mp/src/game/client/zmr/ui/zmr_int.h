#pragma once

class IZMUi
{
public:
    virtual void Create( vgui::VPANEL parent ) = 0;
    virtual void Destroy() = 0;
    virtual vgui::Panel *GetPanel() = 0;
};
