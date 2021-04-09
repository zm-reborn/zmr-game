#include "cbase.h"
#include "filesystem.h"
#include "IGameUIFuncs.h"

#include <vgui/ISurface.h>
#include <vgui/IInput.h>

#include <engine/IEngineSound.h>

#include "zmr_mainmenu_contactbuttons.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


static CDllDemandLoader g_GameUI( "GameUI" );


extern IGameUIFuncs* gameuifuncs;


using namespace vgui;



CON_COMMAND( zm_cl_debug_mainmenu, "" )
{
    auto* pMainMenu = static_cast<CZMMainMenu*>( g_pZMMainMenu->GetPanel() );
    if ( pMainMenu )
    {
        for ( int i = 0 ; i < pMainMenu->GetChildCount(); i++ )
        {
            auto* pChild = pMainMenu->GetChild( i );
            Msg( "Child %i: %s\n", i, pChild->GetName() );
        }
        
    }
}

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
            m_Panel->SetParent( (vgui::Panel*)nullptr );
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
    m_szBottomStrip[0] = NULL;
    m_bInLoadingScreen = false;

    m_pVideoMaterial = nullptr;
    m_pMaterial = nullptr;


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

    m_nTexSectionBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexSectionBgId, "zmr_mainmenu/bg_section", true, false );


    InitVideoBackground();
    


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

void CZMMainMenu::InitVideoBackground()
{
    ReleaseVideoBackground();

    m_pMaterial = nullptr;
    m_pVideoMaterial = g_pVideo->CreateVideoMaterial("VideoMaterial",
                                                     "media/zmr_background01.bik",
                                                     "MOD",
                                                     VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
                                                     VideoSystem::DETERMINE_FROM_FILE_EXTENSION,
                                                     false );
    if ( !m_pVideoMaterial )
    {
        return;
    }

    m_pVideoMaterial->SetLooping( true );
    m_pMaterial = m_pVideoMaterial->GetMaterial();
}

void CZMMainMenu::ReleaseVideoBackground()
{
    if ( !m_pVideoMaterial )
        return;


    g_pVideo->DestroyVideoMaterial( m_pVideoMaterial );
    m_pVideoMaterial = nullptr;
    m_pMaterial = nullptr;
}

void CZMMainMenu::OnInGameStatusChanged( bool bInGame )
{
    CheckInGameButtons( bInGame );

    if ( bInGame )
    {
        ReleaseVideoBackground();
    }
    else
    {
        if ( !m_pVideoMaterial )
        {
            InitVideoBackground();
        }
    }
}

void CZMMainMenu::OnThink()
{
    bool bInGame = engine->IsInGame();
    if ( bInGame && engine->IsLevelMainMenuBackground() )
        bInGame = false;

    if ( s_bWasInGame != bInGame )
    {
        OnInGameStatusChanged( bInGame );

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

void CZMMainMenu::PaintVideoBackground()
{
    // No video to play, so do nothing
    if ( !m_pVideoMaterial )
        return;

    if ( !m_pMaterial )
        return;

    // Update our frame
    if ( !m_pVideoMaterial->Update() )
    {
        // Will get here if not set to loop.
        Assert( 0 );
        return;
    }


    int nWidth, nHeight;
    float u, v;

    m_pVideoMaterial->GetVideoImageSize( &nWidth, &nHeight );
    m_pVideoMaterial->GetVideoTexCoordRange( &u, &v );


    // Draw the polys to draw this out
    CMatRenderContextPtr pRenderContext( materials );

    // Override depth to fix model panel rendering.
    pRenderContext->OverrideDepthEnable( true, false );

    pRenderContext->MatrixMode( MATERIAL_VIEW );
    pRenderContext->PushMatrix();
    pRenderContext->LoadIdentity();

    pRenderContext->MatrixMode( MATERIAL_PROJECTION );
    pRenderContext->PushMatrix();
    pRenderContext->LoadIdentity();

    pRenderContext->Bind( m_pMaterial, nullptr );

    CMeshBuilder meshBuilder;
    auto* pMesh = pRenderContext->GetDynamicMesh( true );
    meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

    float flLeftX = 0;
    float flRightX = nWidth - 1;

    float flTopY = 0;
    float flBottomY = nHeight - 1;

    // Map our UVs to cut out just the portion of the video we're interested in
    float flLeftU = 0.0f;
    float flTopV = 0.0f;

    // We need to subtract off a pixel to make sure we don't bleed
    float flRightU = u - ( 1.0f / (float)nWidth );
    float flBottomV = v - ( 1.0f / (float)nHeight );

    // Get the current viewport size
    int vx, vy, vw, vh;
    pRenderContext->GetViewport( vx, vy, vw, vh );

    // Map from screen pixel coords to -1..1
    flRightX = FLerp( -1, 1, 0, vw, flRightX );
    flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
    flTopY = FLerp( 1, -1, 0, vh , flTopY );
    flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

    float alpha = ( (float)GetFgColor()[3] / 255.0f );

    for ( int corner = 0; corner < 4; corner++ )
    {
        bool bLeft = ( corner == 0 ) || ( corner == 3 );
        meshBuilder.Position3f( ( bLeft ) ? flLeftX : flRightX, ( corner & 2 ) ? flBottomY : flTopY, 0.0f );
        meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
        meshBuilder.TexCoord2f( 0, ( bLeft ) ? flLeftU : flRightU, ( corner & 2 ) ? flBottomV : flTopV );
        meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
        meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
        meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
        meshBuilder.AdvanceVertex();
    }

    meshBuilder.End();
    pMesh->Draw();

    pRenderContext->MatrixMode( MATERIAL_VIEW );
    pRenderContext->PopMatrix();

    pRenderContext->MatrixMode( MATERIAL_PROJECTION );
    pRenderContext->PopMatrix();

    pRenderContext->OverrideDepthEnable( false, false );
}

void CZMMainMenu::PaintBackground()
{
    if ( !s_bWasInGame )
    {
        PaintVideoBackground();
    }

    //
    // Draw the sub button background
    //
#define SECTION_PADDING_SIDES       20
#define SECTION_PADDING_TOP         50
#define SECTION_PADDING_BOTTOM      10
#define SECTION_FADEOUT_TIME        0.25f
#define SECTION_FADEIN_TIME         0.3f

    int len = m_vBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        auto* pButton = m_vBtns[i];


        if ( !pButton->GetSubButtonCount() )
            continue;


        float unarmedtime = pButton->GetUnarmedTime();
        float unarmeddelta = (gpGlobals->realtime - pButton->GetUnarmedTime());

        bool bFadeOut = unarmedtime != 0.0f && unarmeddelta < SECTION_FADEOUT_TIME;


        if ( pButton->IsSubButtonsVisible() || bFadeOut )
        {
            Color clr = m_BgColor;

            int px = pButton->GetXPos();
            int py = pButton->GetYPos();

            int totalheight = pButton->GetSubButtonHeight() * pButton->GetSubButtonCount();


            int start_x = px - SECTION_PADDING_SIDES;
            int start_y = py - totalheight - SECTION_PADDING_TOP;

            int end_x = start_x + (pButton->GetMaxSubTextWidth() + SECTION_PADDING_SIDES*2);
            int end_y = py + SECTION_PADDING_BOTTOM;



            int size_x = end_x - start_x;
            int size_y = end_y - start_y;
            Assert( size_x > 0 );


            int wantedsize = size_x * 2;
            

            // Don't stretch y-axis
            float tex = size_y / (float)wantedsize;


            // Get our alpha
            float armeddelta = gpGlobals->realtime - pButton->GetArmedTime();
            if ( bFadeOut )
            {
                int a = (1.0f - (unarmeddelta / SECTION_FADEOUT_TIME)) * 255;
                if ( a > 255 ) a = 255;

                clr[3] = a;
                Repaint();
            }
            else if ( armeddelta < SECTION_FADEIN_TIME )
            {
                int a = armeddelta / SECTION_FADEIN_TIME * 255;
                if ( a > 255 ) a = 255;

                clr[3] = a;
                Repaint();
            }


            surface()->DrawSetColor( clr );
            surface()->DrawSetTexture( m_nTexSectionBgId );
            surface()->DrawTexturedSubRect( start_x, start_y, end_x, end_y, 0.0f, 0.0f, 1.0f, tex );
        }
    }


    //
    // Draw the "line" strip
    //
    if ( m_iBottomStripChildIndex >= 0 && m_iBottomStripChildIndex < GetChildCount() )
    {
        const int nTexHeight = 512;
        const int nTexWidth = 1024;
        const int w = GetWide();


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
    int len = m_vBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        auto* pButton = m_vBtns[i];
        if ( pButton && (pButton->DrawOnlyInGame() || pButton->DrawOnlyNotInGame()) )
        {
            pButton->SetVisible( pButton->DrawOnlyInGame() ? bInGame : (!bInGame) );
        }
    }
}

void CZMMainMenu::Repaint()
{
    BaseClass::Repaint();
}

void CZMMainMenu::PerformLayout()
{
    BaseClass::PerformLayout();


    int len;

    //
    // Layout the big buttons on the horizontal line when they get auto-sized.
    //
    len = m_vBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        auto* pCur = m_vBtns[i];
        if ( !pCur->IsLayoutHorizontally() )
            continue;

        auto bCurOnlyInGame = pCur->DrawOnlyInGame();
        auto bCurOnlyNotInGame = pCur->DrawOnlyNotInGame();

        // Only layout others if drawn together.
        auto IsDrawnTogether = [ bCurOnlyInGame, bCurOnlyNotInGame ] ( CZMMainMenuButton* pBtn )
        {
            // Always drawn
            if ( !bCurOnlyInGame && !bCurOnlyNotInGame )
                return true;

            if ( pBtn->DrawOnlyInGame() == bCurOnlyInGame )
                return true;

            if ( pBtn->DrawOnlyNotInGame() == bCurOnlyNotInGame )
                return true;

            return false;
        };


        int w, h;
        pCur->GetContentSize( w, h );

        int end_x = pCur->GetXPos() + w + pCur->GetHorizontalMargin();

        // Find the next button(s) on the line
        // and move them further right.
        for ( int j = i + 1; j < len; j++ )
        {
            auto* pNext = m_vBtns[j];
            if ( pNext->IsLayoutHorizontally() )
            {
                if ( pNext->GetXPos() < end_x && IsDrawnTogether( pNext ) )
                {
                    pNext->SetPos( end_x, pNext->GetYPos() );
                }
            }
        }
    }

    //
    // Layout the contact buttons to the bottom left.
    //
    len = GetChildCount();
    for ( int i = 0; i < len; i++ )
    {
        auto* pList = dynamic_cast<CZMMainMenuContactButtonList*>( GetChild( i ) );
        if ( pList )
        {
            pList->SetPos( GetXPos(), GetTall() - pList->GetTall() );
        }
    }
}

void CZMMainMenu::ApplySettings( KeyValues* kv )
{
    Q_strncpy( m_szBottomStrip, kv->GetString( "bottom_strip_start" ), sizeof( m_szBottomStrip ) );


    BaseClass::ApplySettings( kv );
}

void CZMMainMenu::ApplySchemeSettings( IScheme* pScheme )
{
    m_BgColor = GetSchemeColor( "ZMMainMenuBg", COLOR_BLACK, pScheme );

    // By default all elements assume we're not in game.
    s_bWasInGame = false;


    LoadControlSettings( "resource/ui/zmmainmenu.res" );


    m_vBtns.RemoveAll();
    for ( int i = 0; i < GetChildCount(); i++ )
    {
        auto* pButton = dynamic_cast<CZMMainMenuButton*>( GetChild( i ) );
        if ( pButton )
            m_vBtns.AddToTail( pButton );
    }
    

    if ( m_szBottomStrip[0] != NULL )
    {
        m_iBottomStripChildIndex = FindChildIndexByName( m_szBottomStrip );
    }


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
    int len = m_vBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        auto* pButton = m_vBtns[i];
        if ( pButton != pIgnore )
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
