#pragma once

#include <vgui_controls/Panel.h>
#include <vgui/IImage.h>

class CZMMainMenuContactButtonList : public vgui::Panel
{
public:
    typedef vgui::Panel BaseClass;
    typedef CZMMainMenuContactButtonList ThisClass;
    //DECLARE_CLASS( CZMMainMenuContactButtons, vgui::Button );

    CZMMainMenuContactButtonList( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuContactButtonList();


    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;


    void AddButton( KeyValues* kv );
};
