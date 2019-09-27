#include "cbase.h"

#include "ienginevgui.h"
#include "hud.h"
#include "in_buttons.h"
#include "glow_outline_effect.h"
#include "c_team.h"
#include "hud_chat.h"

#include "ui/zmr_textwindow.h"
#include "ui/zmr_scoreboard.h"
#include "ui/zmr_zmview_old.h"
#include "c_zmr_zmvision.h"
#include "c_zmr_util.h"
#include "c_zmr_player.h"
#include "c_zmr_teamkeys.h"
#include "c_zmr_clientmode.h"


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


//
// Move up/down
//
static void IN_ZM_Cmd_MoveUp( const CCommand& args )
{
    bool state = ( args.Arg( 0 )[0] == '+' ) ? true : false;

    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    
    if ( pPlayer )
        pPlayer->SetMouseWheelMove( state ? 1 : 0 );
}
ConCommand zm_cmd_moveup_down( "+zm_cmd_moveup", IN_ZM_Cmd_MoveUp );
ConCommand zm_cmd_moveup_up( "-zm_cmd_moveup", IN_ZM_Cmd_MoveUp );

static void IN_ZM_Cmd_MoveDown( const CCommand& args )
{
    bool state = ( args.Arg( 0 )[0] == '+' ) ? true : false;

    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    
    if ( pPlayer )
        pPlayer->SetMouseWheelMove( state ? -1 : 0 );
}
ConCommand zm_cmd_movedown_down( "+zm_cmd_movedown", IN_ZM_Cmd_MoveDown );
ConCommand zm_cmd_movedown_up( "-zm_cmd_movedown", IN_ZM_Cmd_MoveDown );




//
extern bool PlayerNameNotSetYet( const char *pszName );

void ClientModeZMNormal::FireGameEvent( IGameEvent* pEvent )
{
	if ( Q_strcmp( "player_team", pEvent->GetName() ) == 0 )
	{
        int userid = pEvent->GetInt( "userid" );

        // We have to know if this is the local player or not.
        // Various systems depend on knowing what team we are changing to.
        bool bIsLocalPlayer = false;
        

        
        auto* pPlayer = USERID2PLAYER( userid );

        if ( !pPlayer )
        {
            int iLocalPlayer = engine->GetLocalPlayer();

            if ( iLocalPlayer > 0 )
            {
                player_info_s info;
                engine->GetPlayerInfo( iLocalPlayer, &info );

                bIsLocalPlayer = userid == info.userID;
            }
        }


		bool bDisconnected = pEvent->GetBool( "disconnect" );
		if ( bDisconnected )
			return;


		int team = pEvent->GetInt( "team" );
		bool bAutoTeamed = pEvent->GetInt( "autoteam", false );
		bool bSilent = pEvent->GetInt( "silent", false );

		const char* pszName = pEvent->GetString( "name" );

        //
        // Print to chat
        //
        auto* pHudChat = static_cast<CBaseHudChat*>( GET_HUDELEMENT( CHudChat ) );
		if ( !bSilent && pHudChat && !PlayerNameNotSetYet( pszName ) )
		{
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof(wszPlayerName) );

			wchar_t wszTeam[64];
			C_Team *pTeam = GetGlobalTeam( team );
			if ( pTeam )
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( pTeam->Get_Name(), wszTeam, sizeof(wszTeam) );
			}
			else
			{
				_snwprintf ( wszTeam, sizeof( wszTeam ) / sizeof( wchar_t ), L"%d", team );
			}


            
			wchar_t wszLocalized[100];
			if ( bAutoTeamed )
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_joined_autoteam" ), 2, wszPlayerName, wszTeam );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_joined_team" ), 2, wszPlayerName, wszTeam );
			}

			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

			pHudChat->Printf( CHAT_FILTER_TEAMCHANGE, "%s", szLocalized );
		}


        //
        // Fire local player team change methods.
        // If our local player doesn't exist yet, call static method instead.
        //
		if ( pPlayer && pPlayer->IsLocalPlayer() )
		{
			pPlayer->TeamChange( team );
		}
        else if ( bIsLocalPlayer )
        {
            C_ZMPlayer::TeamChangeStatic( team );
        }

        return;
	}

    BaseClass::FireGameEvent( pEvent );
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
    int ret = g_pZMView->ZMKeyInput( keynum, down );
    if ( ret != -1 )
        return ret;


    // Group select
    if ( down && keynum >= KEY_0 && keynum <= KEY_9 )
    {
        int num = keynum - KEY_0;

        if ( IsZMHoldingCtrl() )
        {
            ZMClientUtil::SetSelectedGroup( num );
        }
        else
        {
            ZMClientUtil::SelectGroup( num );
        }
    }

    return -1;
}
//




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

    virtual IViewPortPanel* CreatePanelByName( const char* szPanelName ) OVERRIDE;
};

using namespace vgui;

//
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
//
