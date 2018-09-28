#include "cbase.h"
#include "filesystem.h"

#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <GameUI/IGameUI.h>


#include "zmr_loadingbg.h"



static CDllDemandLoader g_GameUI( "GameUI" );

using namespace vgui;

class CZMLoadingUI : public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMLoadingUI, Panel );

    CZMLoadingUI( VPANEL parent );
    ~CZMLoadingUI();

    virtual void ApplySchemeSettings( IScheme* pScheme ) OVERRIDE;

    virtual void Paint() OVERRIDE;

    IGameUI* GetGameUI();

private:
    bool LoadGameUI();
    void ReleaseGameUI();


    IGameUI* m_pGameUI;
};

class CZMLoadingUIInterface : public IZMUi
{
public:
    CZMLoadingUIInterface() { m_Panel = nullptr; };

    void Create( VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMLoadingUI( parent );
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
    CZMLoadingUI* m_Panel;
};

static CZMLoadingUIInterface g_ZMLoadingUIInt;
IZMUi* g_pZMLoadingUI = static_cast<IZMUi*>( &g_ZMLoadingUIInt );


CZMLoadingUI::CZMLoadingUI( VPANEL parent ) : BaseClass( nullptr, "ZMMainMenu" )
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

CZMLoadingUI::~CZMLoadingUI()
{
    ReleaseGameUI();
}

IGameUI* CZMLoadingUI::GetGameUI()
{
    return m_pGameUI;
}

bool CZMLoadingUI::LoadGameUI()
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

void CZMLoadingUI::ReleaseGameUI()
{
    g_GameUI.Unload();
    m_pGameUI = nullptr;
}

void CZMLoadingUI::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    int w, h;
    surface()->GetScreenSize( w, h );
    SetSize( w, h );
}

void CZMLoadingUI::Paint()
{
    BaseClass::Paint();

    surface()->DrawSetColor( COLOR_WHITE );
    surface()->DrawFilledRect( 0, 0, ScreenWidth(), ScreenHeight() );
}

void ZMOverrideLoadingUI()
{
    if ( !g_pZMLoadingUI ) return;


    CZMLoadingUI* pMenu = static_cast<CZMLoadingUI*>( g_pZMLoadingUI->GetPanel() );
    
    if ( pMenu->GetGameUI() )
    {
        pMenu->GetGameUI()->SetLoadingBackgroundDialog( pMenu->GetVPanel() );
    }
}
