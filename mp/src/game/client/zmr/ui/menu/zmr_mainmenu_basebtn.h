#pragma once


#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>


class CZMMainMenu;

class CZMMainMenuBaseButton : public vgui::Button
{
public:
    typedef vgui::Button BaseClass;
    typedef CZMMainMenuBaseButton ThisClass;
    //DECLARE_CLASS( CZMMainMenuBaseButton, vgui::Button );
    
    CZMMainMenuBaseButton( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuBaseButton();

    virtual void GetContentSize( int& wide, int& tall ) OVERRIDE;

    virtual void PerformLayout() OVERRIDE;

    virtual void ApplySettings( KeyValues* in ) OVERRIDE;

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

    virtual void Paint() OVERRIDE;

    int GetImageSize();

    void LayoutImage();

    virtual CZMMainMenu* GetMainMenu();


    vgui::IImage* m_pImage;
    int m_iImageX;
    int m_iImageY;
    int m_nImageMargin;
};
