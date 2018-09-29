#pragma once


#include <vgui_controls/EditablePanel.h>
#include <GameUI/IGameUI.h>

#include "zmr/ui/zmr_int.h"



class CZMMainMenu : public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMMainMenu, vgui::EditablePanel );

    CZMMainMenu( vgui::VPANEL parent );
    ~CZMMainMenu();

    virtual void PerformLayout() OVERRIDE;
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    virtual void OnCommand( const char* command ) OVERRIDE;
    virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
    virtual void OnMousePressed( vgui::KeyCode code ) OVERRIDE;

    virtual void PaintBackground() OVERRIDE;



    void HideSubButtons( vgui::Panel* pIgnore = nullptr );

    IGameUI* GetGameUI();

private:
    void PrintDebug();

    bool LoadGameUI();
    void ReleaseGameUI();


    IGameUI* m_pGameUI;

    int m_nTexBgId;
    Color m_BgColor;
};



extern IZMUi* g_pZMMainMenu;

void ZMOverrideGameUI();
