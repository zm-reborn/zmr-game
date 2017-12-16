#include "cbase.h"
#include "filesystem.h"

#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <GameUI/IGameUI.h>


#include "zmr_mainmenu.h"



static CDllDemandLoader g_GameUI( "GameUI" );

using namespace vgui;

class CZMMainMenu : public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMMainMenu, Panel );

    CZMMainMenu( VPANEL parent );
    ~CZMMainMenu();

    virtual void ApplySchemeSettings( IScheme* pScheme ) OVERRIDE;


    IGameUI* GetGameUI();

private:
    bool LoadGameUI();
    void ReleaseGameUI();


    IGameUI* m_pGameUI;
};

class CZMMainMenuInterface : public IZMUi
{
public:
    CZMMainMenuInterface() { m_Panel = nullptr; };

    void Create( VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMMainMenu( parent );
    }
    void Destroy() OVERRIDE
    {
        if ( m_Panel )
        {
            m_Panel->SetParent( nullptr );
            delete m_Panel;
        }
    }
    vgui::Panel* GetPanel() OVERRIDE { return m_Panel; }

private:
    CZMMainMenu* m_Panel;
};

static CZMMainMenuInterface g_ZMMainMenuInt;
IZMUi* g_pZMMainMenu = static_cast<IZMUi*>( &g_ZMMainMenuInt );


CZMMainMenu::CZMMainMenu( VPANEL parent ) : BaseClass( nullptr, "ZMMainMenu" )
{
    SetParent( parent );

    m_pGameUI = nullptr;
    if ( !LoadGameUI() )
    {
        Warning( "Failed to load GameUI!!\n" );
    }


    /*KeyValues* kv = new KeyValues( "GameMenu" );
    kv->UsesEscapeSequences( true );

    if ( kv->LoadFromFile( filesystem, "resource/GameMenu.res" ) )
    {
        
    }*/
}

CZMMainMenu::~CZMMainMenu()
{
    ReleaseGameUI();
}

IGameUI* CZMMainMenu::GetGameUI()
{
    return m_pGameUI;
}

bool CZMMainMenu::LoadGameUI()
{
    if ( m_pGameUI ) return true;


    CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();

    if ( gameUIFactory )
    {
        m_pGameUI = static_cast<IGameUI*>( gameUIFactory( GAMEUI_INTERFACE_VERSION, nullptr ) );


        return m_pGameUI != nullptr;
    }

    return false;
}

void CZMMainMenu::ReleaseGameUI()
{
    g_GameUI.Unload();
    m_pGameUI = nullptr;
}

void CZMMainMenu::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    int w, h;
    surface()->GetScreenSize( w, h );
    SetSize( w, h );
}


void ZMOverrideGameUI()
{
    if ( !g_pZMMainMenu ) return;


    CZMMainMenu* pMenu = static_cast<CZMMainMenu*>( g_pZMMainMenu->GetPanel() );
    
    if ( pMenu->GetGameUI() )
    {
        pMenu->GetGameUI()->SetMainMenuOverride( pMenu->GetVPanel() );
    }
}
