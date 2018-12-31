#include "cbase.h"
#include "filesystem.h"
#include "IGameUIFuncs.h"

#include <vgui/ISurface.h>
#include <vgui/Iinput.h>

#include "zmr_mainmenu_contactbuttons.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


static CDllDemandLoader g_GameUI( "GameUI" );


extern IGameUIFuncs* gameuifuncs;


using namespace vgui;

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


bool CZMMainMenu::s_bWasInGame = false;


CZMMainMenu::CZMMainMenu( VPANEL parent ) : BaseClass( nullptr, "ZMMainMenu" )
{
    m_iBottomStripChildIndex = -1;
    m_bInLoadingScreen = false;


    // Has to be set to load fonts correctly.
    SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" ) );


    SetParent( parent );

    m_pGameUI = nullptr;
    if ( !LoadGameUI() )
    {
        Warning( "Failed to load GameUI!!\n" );
    }



    SetProportional( true );

    SetVisible( true );
    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    


    //MakePopup( false );
    GetFocusNavGroup().SetFocusTopLevel( true );



    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_mainmenu/bg_mainmenu", true, false );


    RequestFocus();
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
    if ( m_pGameUI )
        m_pGameUI->SetMainMenuOverride( NULL );
    m_pGameUI = nullptr;
}

void CZMMainMenu::OnThink()
{
    bool bInGame = engine->IsInGame();
    if ( bInGame && engine->IsLevelMainMenuBackground() )
        bInGame = false;

    if ( s_bWasInGame != bInGame )
    {
        CheckInGameButtons( bInGame );
        s_bWasInGame = bInGame;
    }
}

bool CZMMainMenu::IsVisible()
{
    // We have to have this exception here
    // because just using SetVisible( false ) is not working.
    if ( m_bInLoadingScreen )
        return false;


    return BaseClass::IsVisible();
}

void CZMMainMenu::PaintBackground()
{
    // Draw the "line" strip
    const int nTexHeight = 512;
    const int nTexWidth = 1024;
    const int w = GetWide();

    if ( m_iBottomStripChildIndex >= 0 && m_iBottomStripChildIndex < GetChildCount() )
    {
        auto* pPanel = GetChild( m_iBottomStripChildIndex );


        surface()->DrawSetColor( m_BgColor );

        
        int h = pPanel ? (pPanel->GetTall()+10) : 100;

        int offset_y = pPanel ? ((pPanel->GetYPos()+pPanel->GetTall()/2)-h/2) : 0;

        if ( (offset_y + h) > GetTall() )
        {
            h = GetTall() - offset_y;
        }


        float repeats = w / (nTexWidth * (h / (float)nTexHeight));


        surface()->DrawSetTexture( m_nTexBgId );
        surface()->DrawTexturedSubRect( 0, offset_y, w, offset_y + h, 0.0f, 0.0f, repeats, 1.0f );
    }
}

void CZMMainMenu::CheckInGameButtons( bool bInGame )
{
    int len = GetChildCount();
    for ( int i = 0; i < len; i++ )
    {
        auto* pButton = dynamic_cast<CZMMainMenuButton*>( GetChild( i ) );
        if ( pButton && pButton->DrawOnlyInGame() )
        {
            pButton->SetVisible( bInGame );
        }
    }
}

void CZMMainMenu::PerformLayout()
{
    // Layout the child buttons accordingly
    int len = GetChildCount();
    for ( int i = 0; i < len; i++ )
    {
        Panel* pChild = GetChild( i );

        auto* pList = dynamic_cast<CZMMainMenuContactButtonList*>( pChild );
        if ( pList )
        {
            pChild->SetPos( GetXPos(), GetTall() - GetChild( i )->GetTall() );

            continue;
        }
    }

    BaseClass::PerformLayout();
}

void CZMMainMenu::ApplySettings( KeyValues* kv )
{
    m_iBottomStripChildIndex = kv->GetInt( "bottom_strip_index", -1 );

    BaseClass::ApplySettings( kv );
}

void CZMMainMenu::ApplySchemeSettings( IScheme* pScheme )
{
    m_BgColor = GetSchemeColor( "ZMMainMenuBg", COLOR_BLACK, pScheme );


    LoadControlSettings( "resource/ui/zmmainmenu.res" );
    

    int w, h;
    surface()->GetScreenSize( w, h );
    SetBounds( 0, 0, w, h );


    BaseClass::ApplySchemeSettings( pScheme );
}

void CZMMainMenu::OnCommand( const char* command )
{
    m_pGameUI->SendMainMenuCommand( command );
    //BaseClass::OnCommand( command );
}

void CZMMainMenu::OnKeyCodePressed( KeyCode code )
{
    // ZMRTODO: We never seem to actually receive any codes... :(

    // HACK: Allow F key bindings to operate even here
    //if ( IsPC() && code >= KEY_F1 && code <= KEY_F12 )
    //{
    //    // See if there is a binding for the FKey
    //    const char* binding = gameuifuncs->GetBindingForButtonCode( code );
    //    if ( binding && *binding )
    //    {
    //        // submit the entry as a console commmand
    //        char szCommand[256];
    //        Q_strncpy( szCommand, binding, sizeof( szCommand ) );
    //        engine->ClientCmd_Unrestricted( szCommand );
    //    }
    //}

    BaseClass::OnKeyCodePressed( code );
}

void CZMMainMenu::OnMousePressed( KeyCode code )
{
    HideSubButtons();
    

    BaseClass::OnMousePressed( code );
}

void CZMMainMenu::HideSubButtons( CZMMainMenuButton* pIgnore )
{
    int len = GetChildCount();
    for ( int i = 0; i < len; i++ )
    {
        auto* pButton = dynamic_cast<CZMMainMenuButton*>( GetChild( i ) );
        if ( pButton && pButton != pIgnore )
        {
            pButton->SetSelected( false );
        }
    }
}

void CZMMainMenu::OnLoadingScreenStart()
{
    SetLoadingScreenState( true );
}

void CZMMainMenu::OnLoadingScreenEnd()
{
    SetLoadingScreenState( false );
}

void CZMMainMenu::SetLoadingScreenState( bool state )
{
    // Yea, we have to manually hide the main menu when we start loading.
    bool draw = !state;

    m_bInLoadingScreen = state;
    SetVisible( draw );
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
