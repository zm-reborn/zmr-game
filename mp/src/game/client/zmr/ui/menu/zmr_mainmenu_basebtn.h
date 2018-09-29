#pragma once


#include <vgui_controls/Button.h>


class CZMMainMenu;

class CZMMainMenuBaseButton : public vgui::Button
{
public:
    typedef vgui::Button BaseClass;
    typedef CZMMainMenuBaseButton ThisClass;
    //DECLARE_CLASS( CZMMainMenuBaseButton, vgui::Button );
    
    CZMMainMenuBaseButton( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuBaseButton();


    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;


    virtual CZMMainMenu* GetMainMenu();
};
