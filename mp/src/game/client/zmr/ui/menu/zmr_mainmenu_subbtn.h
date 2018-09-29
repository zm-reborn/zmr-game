#pragma once


#include "zmr_mainmenu_basebtn.h"


class CZMMainMenu;

class CZMMainMenuSubButton : public CZMMainMenuBaseButton
{
public:
    typedef CZMMainMenuBaseButton BaseClass;
    typedef CZMMainMenuSubButton ThisClass;
    //DECLARE_CLASS( CZMMainMenuButton, CZMMainMenuBaseButton );

    CZMMainMenuSubButton( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuSubButton();


    virtual void Paint() OVERRIDE;

    virtual void FireActionSignal() OVERRIDE;
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;


    void FadeIn( float fade );
    void FadeOut( float fade );

    bool IsFadingOut() const { return m_flFadeOut != 0.0f; }
    bool IsFadingIn() const { return m_flFadeIn != 0.0f; }

private:
    vgui::Panel* m_pButtonParent;

    float m_flFadeIn;
    float m_flFadeInTime;
    float m_flFadeOut;
    float m_flFadeOutTime;
};
