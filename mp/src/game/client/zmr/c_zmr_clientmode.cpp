#include "cbase.h"
#include "clientmode_shared.h"
#include "ienginevgui.h"
#include "glow_outline_effect.h"

#include "ui/zmr_spectator_ui.h"
#include "ui/zmr_textwindow.h"
#include "ui/zmr_scoreboard.h"
#include "c_zmr_zmvision.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


bool g_bRenderPostProcess = false;


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
};


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
    static ClientModeZMNormal g_ClientModeNormal;
    return &g_ClientModeNormal;
}


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
    IViewPortPanel* newpanel = NULL;

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
    else if ( Q_strcmp( PANEL_SPECGUI, szPanelName ) == 0 )
    {
        //newpanel = new CZMHudSpectatorUI( this );
        return newpanel;
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
}
