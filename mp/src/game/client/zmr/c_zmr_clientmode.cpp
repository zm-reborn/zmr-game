#include "cbase.h"
#include "clientmode_shared.h"
#include "ienginevgui.h"
#include "glow_outline_effect.h"

#include "zmr/ui/zmr_spectator_ui.h"
#include "zmr/ui/zmr_textwindow.h"
#include "zmr/ui/zmr_scoreboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

class ClientModeZMNormal : public ClientModeShared
{
public:
    DECLARE_CLASS( ClientModeZMNormal, ClientModeShared );

    ClientModeZMNormal();
    ~ClientModeZMNormal();

    virtual void Init() OVERRIDE;

    virtual bool DoPostScreenSpaceEffects( const CViewSetup* pSetup );
};


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
    static ClientModeZMNormal g_ClientModeNormal;
    return &g_ClientModeNormal;
}


bool ClientModeZMNormal::DoPostScreenSpaceEffects( const CViewSetup* pSetup )
{
    g_GlowObjectManager.RenderGlowEffects( pSetup, 0 );

    return true;
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
        newpanel = new CZMSpectatorGUI( this );	
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
