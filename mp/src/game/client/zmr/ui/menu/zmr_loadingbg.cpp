#include "cbase.h"
#include "filesystem.h"

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui/ISurface.h>
#include <GameUI/IGameUI.h>
#include <ienginevgui.h>
//#include "game_controls/vguitextwindow.h"

#include "c_zmr_legacy_objpanel.h"
#include "c_zmr_tips.h"
#include "c_zmr_music.h"

#include "zmr_mainmenu.h"
#include "zmr_loadingbg.h"



static CDllDemandLoader g_GameUI( "GameUI" );


class CZMLoadingScreenText : public vgui::TextEntry
{
public:
    DECLARE_CLASS_SIMPLE( CZMLoadingScreenText, vgui::TextEntry );


    CZMLoadingScreenText( vgui::Panel* pParent, const char* name );
    ~CZMLoadingScreenText();

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
};

DECLARE_BUILD_FACTORY( CZMLoadingScreenText );

CZMLoadingScreenText::CZMLoadingScreenText( vgui::Panel* pParent, const char* name ) : vgui::TextEntry( pParent, name )
{
}

CZMLoadingScreenText::~CZMLoadingScreenText()
{
}

void CZMLoadingScreenText::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    SetBorder( nullptr );
    SetFgColor( COLOR_WHITE );
}


class CZMLoadingPanel : public vgui::EditablePanel, public CGameEventListener
{
public:
    DECLARE_CLASS_SIMPLE( CZMLoadingPanel, vgui::EditablePanel );

    CZMLoadingPanel( vgui::VPANEL parent );
    ~CZMLoadingPanel();


    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

    virtual void PaintBackground() OVERRIDE;
    virtual void Paint() OVERRIDE;


    MESSAGE_FUNC( OnActivate, "activate" );
    MESSAGE_FUNC( OnDeactivate, "deactivate" );


    IGameUI* GetGameUI() const;

private:
    bool LoadGameUI();
    void ReleaseGameUI();


    void ChangeTip();

    void SetMapNameText( const char* mapname );
    void SetTipText( const char* msg );


    void FindLoadingPanel();
    vgui::VPANEL m_loadingDialog;


    vgui::Label* m_pMapNameLabel;
    vgui::Label* m_pTipLabel;

    CZMLoadingScreenText* m_pTextMessage;

    bool m_bHasMapName;

    int m_nTexBgId;
    int m_nTexStripBgId;
    int m_nTexTipBgId;
    Color m_BgColor;

    IGameUI* m_pGameUI;

    bool m_bActive;
};

class CZMLoadingPanelInterface : public IZMUi
{
public:
    CZMLoadingPanelInterface() { m_Panel = nullptr; };

    void Create( vgui::VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMLoadingPanel( parent );
    }
    void Destroy() OVERRIDE
    {
        if ( m_Panel )
        {
            m_Panel->SetParent( (vgui::Panel*)nullptr );
            delete m_Panel;
        }
    }
    vgui::Panel* GetPanel() OVERRIDE { return m_Panel; }

private:
    CZMLoadingPanel* m_Panel;
};


using namespace vgui;

static CZMLoadingPanelInterface g_ZMLoadingUIInt;
IZMUi* g_pZMLoadingUI = static_cast<IZMUi*>( &g_ZMLoadingUIInt );


CZMLoadingPanel::CZMLoadingPanel( VPANEL parent ) : BaseClass( nullptr, "ZMLoadingPanel" )
{
    m_loadingDialog = NULL;

    m_bActive = false;

    m_bHasMapName = false;

    m_pMapNameLabel = new Label( this, "MapNameLabel", "" );
    m_pTipLabel = new Label( this, "TipLabel", "" );


	m_pTextMessage = new CZMLoadingScreenText( this, "TextMessage" );
    m_pTextMessage->SetMultiline( true );
    m_pTextMessage->SetVerticalScrollbar( false );



    // Has to be set to load fonts correctly.
    SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" ) );


    SetProportional( true );


    SetParent( parent );

    m_pGameUI = nullptr;
    if ( !LoadGameUI() )
    {
        Warning( "Failed to load GameUI!!\n" );
    }


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_mainmenu/bg_mainmenu", true, false );

    m_nTexStripBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexStripBgId, "zmr_mainmenu/strip_zombies", true, false );

    m_nTexTipBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexTipBgId, "zmr_mainmenu/bg_tip", true, false );


    ListenForGameEvent( "server_spawn" );
}

CZMLoadingPanel::~CZMLoadingPanel()
{
    ReleaseGameUI();
}

static void GetMapDisplayName( char* buffer, int len );

void CZMLoadingPanel::FireGameEvent( IGameEvent* pEvent )
{
    if ( !Q_strcmp( pEvent->GetName(), "server_spawn" ) )
    {
        const char* mapname = pEvent->GetString( "mapname" );
        if ( mapname && *mapname )
        {
            bool bValid = !engine->IsLevelMainMenuBackground();

            char fixedname[128];
            Q_strncpy( fixedname, mapname, sizeof( fixedname ) );
            GetMapDisplayName( fixedname, sizeof( fixedname ) );

            SetMapNameText( bValid ? fixedname : "" );


            char* buffer = new char[4096];
            buffer[0] = NULL;
            ZMLegacyObjPanel::LoadObjectivesFromFile( mapname, buffer, 4096 );

            if ( *buffer )
            {
	            m_pTextMessage->SetVisible( true );
	            m_pTextMessage->SetText( buffer );
	            m_pTextMessage->GotoTextStart();
            }


            delete[] buffer;
        }
    }
}

IGameUI* CZMLoadingPanel::GetGameUI() const
{
    return m_pGameUI;
}

bool CZMLoadingPanel::LoadGameUI()
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

void CZMLoadingPanel::ReleaseGameUI()
{
    g_GameUI.Unload();
    m_pGameUI = nullptr;
}

void CZMLoadingPanel::ApplySchemeSettings( IScheme* pScheme )
{
    m_BgColor = GetSchemeColor( "ZMLoadingBg", COLOR_BLACK, pScheme );


    LoadControlSettings( "resource/ui/zmloadingpanel.res" );


    BaseClass::ApplySchemeSettings( pScheme );


    int w, h;
    surface()->GetScreenSize( w, h );
    SetBounds( 0, 0, w, h );
}

void CZMLoadingPanel::OnActivate()
{
    if ( m_bActive )
    {
        //
        // EXTREME SUPER DUPER HACK:
        // Apparently this is called AGAIN if there's an error.
        // Error will be displayed in the center, so hide the map text
        // so the error can be seen fully.
        //
        m_pTextMessage->SetVisible( false );


        return;
    }

    m_bActive = true;


    SetMapNameText( "" );

    m_pTextMessage->SetVisible( false );

    ChangeTip();

#ifdef ZMR_FINAL
    ZMMusic::g_ZMMusicManager.SetMusicState( ZMMusic::MUSICSTATE_LOADINGSCREEN );
#endif


    // Tell main menu we're being drawn.
    auto mainMenuPanel = g_pZMMainMenu->GetPanel()->GetVPanel();
    auto myPanel = GetVPanel();
    ipanel()->SendMessage( mainMenuPanel, new KeyValues( "loadingstart" ), myPanel );


    // Find the loading dialog so we can draw it manually.
    FindLoadingPanel();

    surface()->RestrictPaintToSinglePanel( GetVPanel() );
}

void CZMLoadingPanel::OnDeactivate()
{
    if ( !m_bActive )
    {
        Assert( 0 );
        return;
    }

    m_bActive = false;


#ifdef ZMR_FINAL
    ZMMusic::g_ZMMusicManager.SetMusicState( ZMMusic::MUSICSTATE_NONE );
#endif

    // Tell main menu we're no longer drawn.
    auto mainMenuPanel = g_pZMMainMenu->GetPanel()->GetVPanel();
    auto myPanel = GetVPanel();
    ipanel()->SendMessage( mainMenuPanel, new KeyValues( "loadingend" ), myPanel );

    surface()->RestrictPaintToSinglePanel( NULL );
}

void CZMLoadingPanel::PaintBackground()
{
    //BaseClass::PaintBackground();


    surface()->DrawSetColor( m_BgColor );

    {
        // Draw the "line" strip
        int nTexHeight = 512;
        const int nTexWidth = 1024;
        const int w = GetWide();

        float repeats;



        int h = m_pMapNameLabel->GetTall();

        int offset_y = m_pMapNameLabel->GetYPos();

        if ( (offset_y + h) > GetTall() )
        {
            h = GetTall() - offset_y;
        }


        repeats = w / (nTexWidth * (h / (float)nTexHeight));

        surface()->DrawSetTexture( m_nTexBgId );
        surface()->DrawTexturedSubRect( 0, offset_y, w, offset_y + h, 0.0f, 0.0f, repeats, 1.0f );


        // Draw the zombie strip 1024x256
        h = offset_y + 20;
        offset_y = 0;

        nTexHeight = 256;

        repeats = w / (nTexWidth * (h / (float)nTexHeight));

        surface()->DrawSetTexture( m_nTexStripBgId );
        surface()->DrawTexturedSubRect( 0, offset_y, w, offset_y + h, 0.0f, 0.0f, repeats, 1.0f );
    }


    {
        // Tip background
        //int padding_x = 20;
        //int padding_y = 20;

        //int x = m_pTipLabel->GetXPos();
        //int y = m_pTipLabel->GetYPos();

        //int w, h;
        //m_pTipLabel->GetContentSize( w, h );
        ////w += m_pTipLabel->GetXPos();


        //surface()->DrawSetTexture( m_nTexTipBgId );
        //surface()->DrawTexturedRect( x - padding_x, y - padding_y, x + w + padding_x, y + h + padding_y );
    }
}

void CZMLoadingPanel::Paint()
{
    BaseClass::Paint();


    // Unfortunately, we have to draw the loading dialog manually,
    // because we have to restrict the drawing to this panel, because
    // other main menu panels will draw on top of us.
    // Blah.
    if ( m_loadingDialog != NULL )
    {
        surface()->RestrictPaintToSinglePanel( NULL );

        //int prevZ = ipanel()->GetZPos( m_loadingDialog );

        //ipanel()->SetZPos( m_loadingDialog, 1337 );
        ipanel()->PaintTraverse( m_loadingDialog, true );
        //ipanel()->SetZPos( m_loadingDialog, prevZ );


        surface()->RestrictPaintToSinglePanel( GetVPanel() );
    }
}

void CZMLoadingPanel::FindLoadingPanel()
{
    VPANEL root = enginevgui->GetPanel( VGuiPanel_t::PANEL_GAMEUIDLL );

    if ( root )
    {
        VPANEL panel1, panel2;
        const char* name;
        for ( int i = 0; i < ipanel()->GetChildCount( root ); i++ )
        {
            panel1 = ipanel()->GetChild( root, i );
            if ( Q_stricmp( ipanel()->GetName( panel1 ), "BaseGameUIPanel" ) == 0 )
            {
                for ( int j = 0; j < ipanel()->GetChildCount( panel1 ); j++ )
                {
                    panel2 = ipanel()->GetChild( panel1, j );
                    name = ipanel()->GetName( panel2 );
                    if ( Q_stricmp( name, "LoadingDialog" ) == 0 )
                    {
                        m_loadingDialog = panel2;
                        return;
                    }
                }

                return;
            }
        }
    }
}

void CZMLoadingPanel::SetMapNameText( const char* mapname )
{
    m_bHasMapName = mapname && *mapname;


    auto* pLabel = static_cast<Label*>( FindChildByName( "MapNameLabel" ) );
    if ( pLabel )
    {
        pLabel->SetText( mapname );
    }
}

void CZMLoadingPanel::SetTipText( const char* msg )
{
    auto* pLabel = static_cast<Label*>( FindChildByName( "TipLabel" ) );
    if ( pLabel )
    {
        pLabel->SetText( msg );
    }
}

void CZMLoadingPanel::ChangeTip()
{
    auto* pTipManager = g_ZMTipSystem.GetManagerByIndex( ZMTIPMANAGER_LOADINGSCREEN );
    Assert( pTipManager );

    if ( !pTipManager )
        return;


    const CZMTip* pTip = nullptr;

    ZMConstTips_t tips;

    
    ZMConstTips_t vTips;
    pTipManager->GetTips( vTips );

    FOR_EACH_VEC( vTips, i )
    {
        if ( !vTips[i]->ShowRandom() && vTips[i]->ShouldShow() )
        {
            tips.AddToTail( vTips[i] );
        }
    }


    if ( !tips.Count() )
    {
        FOR_EACH_VEC( vTips, i )
        {
            if ( vTips[i]->ShouldShow() )
            {
                tips.AddToTail( vTips[i] );
            }
        }
    }


    if ( tips.Count() )
    {
        pTip = tips[random->RandomInt( 0, tips.Count() -1 )];
    }
    

    if ( pTip )
    {
        char buffer[512];
        pTip->FormatMessage( buffer, sizeof( buffer ) );

        SetTipText( buffer );
    }
}


static void GetMapDisplayName( char* buffer, int len )
{
    char cpy[128];
    const char* start = buffer;

    // Skip the prefix
    if ( Q_stristr( buffer, "zm_" ) == buffer )
    {
        start += 3;
    }
    else if ( Q_stristr( buffer, "zmr_" ) == buffer )
    {
        start += 4;
    }


    Q_strncpy( cpy, start, sizeof( cpy ) );

    // Replace underscore with space
    char* us = cpy;
    while ( (us = Q_strstr( us, "_" )) != nullptr )
    {
        *us = ' ';
        ++us;
    }


    Q_strncpy( buffer, cpy, len );
}


void ZMOverrideLoadingUI()
{
    if ( !g_pZMLoadingUI ) return;


    CZMLoadingPanel* pMenu = static_cast<CZMLoadingPanel*>( g_pZMLoadingUI->GetPanel() );
    
    if ( pMenu->GetGameUI() )
    {
        pMenu->GetGameUI()->SetLoadingBackgroundDialog( pMenu->GetVPanel() );
    }
}
