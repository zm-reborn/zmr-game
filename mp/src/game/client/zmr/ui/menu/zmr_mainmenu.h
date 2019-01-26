#pragma once


#include <vgui_controls/EditablePanel.h>
#include <GameUI/IGameUI.h>

#include "zmr/ui/zmr_int.h"


class CZMMainMenuButton;


class CZMMainMenu : public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMMainMenu, vgui::EditablePanel );

    CZMMainMenu( vgui::VPANEL parent );
    ~CZMMainMenu();

    virtual void OnThink() OVERRIDE;
    virtual void PerformLayout() OVERRIDE;
    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    virtual void OnCommand( const char* command ) OVERRIDE;
    virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
    virtual void OnMousePressed( vgui::KeyCode code ) OVERRIDE;

    virtual void PaintBackground() OVERRIDE;

    virtual bool IsVisible() OVERRIDE;


    MESSAGE_FUNC( OnLoadingScreenStart, "loadingstart" );
    MESSAGE_FUNC( OnLoadingScreenEnd, "loadingend" );


    void HideSubButtons( CZMMainMenuButton* pIgnore = nullptr );

    IGameUI* GetGameUI();

private:
    bool LoadGameUI();
    void ReleaseGameUI();

    void SetLoadingScreenState( bool state );


    void CheckInGameButtons( bool bInGame );
    static bool s_bWasInGame;


    IGameUI* m_pGameUI;

    int m_nTexBgId;
    Color m_BgColor;


    bool m_bInLoadingScreen;

    int m_iBottomStripChildIndex;
    char m_szBottomStrip[64];
};



extern IZMUi* g_pZMMainMenu;

void ZMOverrideGameUI();
