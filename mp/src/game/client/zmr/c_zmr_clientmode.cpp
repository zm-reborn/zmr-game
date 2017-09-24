#include "cbase.h"
#include "clientmode_shared.h"
#include "hl2mptextwindow.h"
#include "ienginevgui.h"

#include "zmr/ui/zmr_scoreboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


HScheme g_hVGuiCombineScheme = 0;



class ClientModeZMNormal : public ClientModeShared
{
public:
    DECLARE_CLASS( ClientModeZMNormal, ClientModeShared );

    ClientModeZMNormal();
    ~ClientModeZMNormal();

    virtual void Init() OVERRIDE;
};


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
    static ClientModeZMNormal g_ClientModeNormal;
    return &g_ClientModeNormal;
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
        newpanel = new CHL2MPTextWindow( this );
        return newpanel;
    }
    else if ( Q_strcmp( PANEL_SPECGUI, szPanelName ) == 0 )
    {
        newpanel = new CHL2MPSpectatorGUI( this );	
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

    // Load up the combine control panel scheme
    g_hVGuiCombineScheme = scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/CombinePanelScheme.res", "CombineScheme" );
    
    if ( !g_hVGuiCombineScheme )
    {
        Warning( "Couldn't load combine panel scheme!\n" );
    }
}
