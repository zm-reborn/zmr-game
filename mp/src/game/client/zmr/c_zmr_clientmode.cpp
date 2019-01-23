#include "cbase.h"
#include "clientmode_shared.h"
#include "ienginevgui.h"
#include "hud.h"
#include "in_buttons.h"
#include "glow_outline_effect.h"

#include "ui/zmr_textwindow.h"
#include "ui/zmr_scoreboard.h"
#include "ui/zmr_zmview_old.h"
#include "c_zmr_zmvision.h"
#include "c_zmr_util.h"
#include "c_zmr_player.h"
#include "c_zmr_zmkeys.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


bool g_bRenderPostProcess = false;



ConVar zm_cl_usenewzmview( "zm_cl_usenewzmview", "0", FCVAR_ARCHIVE, "Use new ZM view HUD?" );

CZMViewBase* GetZMView()
{
    if ( zm_cl_usenewzmview.GetBool() )
    {
        Assert( 0 );
        return nullptr;
    }
    else
    {
        static CZMViewOld* pOld = new CZMViewOld( "ZMView" );
        return pOld;
    }
}


using namespace vgui;

class ClientModeZMNormal : public ClientModeShared
{
public:
    DECLARE_CLASS( ClientModeZMNormal, ClientModeShared );

    ClientModeZMNormal();
    ~ClientModeZMNormal();

    virtual void Init() OVERRIDE;

    virtual bool DoPostScreenSpaceEffects( const CViewSetup* pSetup ) OVERRIDE;
    virtual void PostRender() OVERRIDE;

    virtual int KeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding );


    bool IsZMHoldingCtrl() const { return m_bZMHoldingCtrl; }
    void SetZMHoldingCtrl( bool state ) { m_bZMHoldingCtrl = state; }

private:
    int ZMKeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding );

    bool m_bZMHoldingCtrl;
};


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
    static ClientModeZMNormal g_ClientModeNormal;
    return &g_ClientModeNormal;
}

ClientModeZMNormal* GetZMClientMode()
{
    return static_cast<ClientModeZMNormal*>( GetClientModeNormal() );
}


static void IN_ZM_Cmd_Control( const CCommand& args )
{
    bool state = ( args.Arg( 0 )[0] == '+' ) ? true : false;

    GetZMClientMode()->SetZMHoldingCtrl( state );
}
ConCommand zm_cmd_ctrl_up( "+zm_cmd_ctrl", IN_ZM_Cmd_Control );
ConCommand zm_cmd_ctrl_down( "-zm_cmd_ctrl", IN_ZM_Cmd_Control );


bool ClientModeZMNormal::DoPostScreenSpaceEffects( const CViewSetup* pSetup )
{
    // Makes sure we don't redraw character circles here.
    g_bRenderPostProcess = true;


    g_GlowObjectManager.RenderGlowEffects( pSetup, 0 );

    g_ZMVision.RenderSilhouette();


    g_bRenderPostProcess = false;


    return true;
}

void ClientModeZMNormal::PostRender()
{
    g_ZMVision.UpdateLight();

    BaseClass::PostRender();
}

int ClientModeZMNormal::KeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding )
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    const bool bIsZM = pPlayer && pPlayer->IsZM();


    if ( bIsZM )
    {
        int ret = ZMKeyInput( down, keynum, pszCurrentBinding );

        if ( ret != -1 )
            return ret;
    }


    return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
}

// Return -1 to call baseclass.
int ClientModeZMNormal::ZMKeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding )
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();



    // Group select
    if ( down && keynum >= KEY_0 && keynum <= KEY_9 )
    {
        int group = keynum - KEY_0;
        if ( IsZMHoldingCtrl() )
        {
            ZMClientUtil::SetSelectedGroup( group );
        }
        else
        {
            ZMClientUtil::SelectGroup( group );
        }
    }


    // Mousewheel move
    // We have to put this here or otherwise we can't move while in free-cam.
    if ( down && (keynum == MOUSE_WHEEL_DOWN || keynum == MOUSE_WHEEL_UP) )
    {
        pPlayer->SetMouseWheelMove( ( keynum == MOUSE_WHEEL_DOWN ) ? -1.0f : 1.0f );
    }

    return -1;
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CZMViewport : public CBaseViewport
{
public:
    DECLARE_CLASS_SIMPLE( CZMViewport, CBaseViewport );


    virtual void ApplySchemeSettings( IScheme* pScheme ) OVERRIDE
    {
        BaseClass::ApplySchemeSettings( pScheme );

        gHUD.InitColors( pScheme );

        SetPaintBackgroundEnabled( false );
    }

    virtual IViewPortPanel* CreatePanelByName( const char *szPanelName ) OVERRIDE;
};

IViewPortPanel* CZMViewport::CreatePanelByName( const char* szPanelName )
{
    IViewPortPanel* newpanel = nullptr;

    if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName ) == 0 )
    {
        // Our own scoreboard.
        newpanel = new CZMClientScoreBoardDialog( this );
        return newpanel;
    }
    else if ( Q_strcmp( PANEL_INFO, szPanelName ) == 0 )
    {
        newpanel = new CZMTextWindow( this );
        return newpanel;
    }
    else if ( Q_strcmp( PANEL_SPECGUI, szPanelName ) == 0 ) // Our spectator gui (black bars) are now hud elements.
    {
        //newpanel = new CZMHudSpectatorUI( this );
        return newpanel;
    }
    else if ( Q_strcmp( PANEL_SPECMENU, szPanelName ) == 0 ) // Never create spectator menu.
    {
        return nullptr;
    }

    
    return BaseClass::CreatePanelByName( szPanelName ); 
}

ClientModeZMNormal::ClientModeZMNormal()
{
    m_pViewport = new CZMViewport();
    m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModeZMNormal::~ClientModeZMNormal()
{
}

void ClientModeZMNormal::Init()
{
    BaseClass::Init();


    DevMsg( "Adding ZM view to hud element list\n" );

    CZMViewBase* pView = GetZMView();
    if ( pView )
    {
        gHUD.RemoveHudElement( pView );
        gHUD.AddHudElement( pView );

        g_pZMView = pView;
    }
}
