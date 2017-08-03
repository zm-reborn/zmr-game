#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>
#include <vgui_controls/ImagePanel.h>

#include "filesystem.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>

#include "vgui_bitmapbutton.h"

#include "IGameUIFuncs.h" // for key bindings
#include <igameresources.h>

#include "c_user_message_register.h"


extern IGameUIFuncs *gameuifuncs; // for key binding details

//#include <iviewport.h>
/*
#include <stdlib.h> // MAX_PATH define
#include <stdio.h>*/

#include "iclientmode.h"


#include "zmr/c_zmr_entities.h"
#include "zmr_manimenu.h"
#include "zmr_buildmenu.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player_shared.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_viewport.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




using namespace vgui;

CZMManiMenu *g_pManiMenu = nullptr;

CZMManiMenu::CZMManiMenu( IViewPort *pViewPort ) : Frame( g_pClientMode->GetViewport(), "ZMManiMenu" )
{
	g_pManiMenu = this;

	m_pViewPort = pViewPort;


    if ( g_pZMView )
    {
        SetParent( g_pZMView->GetVPanel() );
    }


    SetTrapIndex( 0 );

    // NOTE: You have to set these before invalidating layout(?)
    SetSizeable( true );
    SetProportional( false );
    SetMoveable( true );
    


	scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ZombieMaster.res", "ZombieMaster" );
	SetScheme( scheme );


	m_hLargeFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Trebuchet18", true );
	m_hMediumFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Trebuchet16", true );

	

	LoadControlSettings( "resource/ui/zmmanimenu.res" );
    InvalidateLayout();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );


    Button* button = dynamic_cast<Button*>( FindChildByName( "Description" ) );
    if ( button ) button->SetAsDefaultButton( 1 );


    

    // Clip to view.
    int w, h;
    GetSize( w, h );

    int x, y;
    GetPos( x, y );

    int newx, newy;

    newx = x;
    newy = y;

    if ( x < 0 ) newx = 0;
    if ( y < 0 ) newy = 0;

    if ( (x + w) > ScreenWidth() ) newx = ScreenWidth() - w;
    if ( (y + h) > ScreenHeight() ) newy = ScreenHeight() - h;


    SetPos( newx, newy );
}

CZMManiMenu::~CZMManiMenu()
{

}

void CZMManiMenu::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;

    
    if ( state )
    {
        // We have to hide the other or otherwise it will cause focus fighting.
        if ( g_pBuildMenu )
        {
            g_pBuildMenu->ShowPanel( false );
        }
    }


    SetVisible( state );
}

void CZMManiMenu::SetDescription( const char* desc )
{
    Label* entry = dynamic_cast<Label*>( FindChildByName( "Description" ) );
    
    if ( entry )
    {
        entry->SetText( desc );
    }
}

void CZMManiMenu::SetCost( int cost )
{
    char buffer[128];
    Q_snprintf( buffer, sizeof( buffer ), "Activate for %i resources.",  cost );

    Label* entry = dynamic_cast<Label*>( FindChildByName( "Activate" ) );
    
    if ( entry )
    {
        entry->SetText( buffer );
    }

    m_nCost = cost;
}

void CZMManiMenu::SetTrapCost( int cost )
{
    char buffer[128];

    if ( cost > 0 )
    {
        Q_snprintf( buffer, sizeof( buffer ), "Create trap for %i resources.",  cost );
    }
    else
    {
        // Are traps gay?
        Q_snprintf( buffer, sizeof( buffer ), "Traps not allowed.",  cost );
    }
    

    Label* entry = dynamic_cast<Label*>( FindChildByName( "Trap" ) );
    
    if ( entry )
    {
        entry->SetText( buffer );
    }

    m_nTrapCost = cost;
}

void CZMManiMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "manipulate" ) == 0 )
	{
		engine->ClientCmd( VarArgs( "zm_cmd_trigger %i", GetTrapIndex() ) );
	}
    else if ( Q_stricmp( command, "trap" ) == 0 )
    {
        if ( g_pZMView )
            g_pZMView->SetClickMode( ZMCLICKMODE_TRAP );
    }
    
    Close();

	BaseClass::OnCommand( command );
}

void CZMManiMenu::OnThink()
{
	if ( !IsVisible() ) return;


    MoveToFront();


	C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer || !pPlayer->IsZM() )
	{
        return;
	}


    SetControlEnabled( "Activate", ( pPlayer->GetResources() >= m_nCost ) );
    SetControlEnabled( "Trap", ( m_nTrapCost > 0 && pPlayer->GetResources() >= m_nTrapCost ) );
}

void __MsgFunc_ZMManiMenuUpdate( bf_read &msg )
{
    if ( !g_pManiMenu ) return;


    // The button will allocate memory.
    char desc[256];
    msg.ReadString( desc, sizeof( desc ), true );


    g_pManiMenu->SetDescription( desc );
}

USER_MESSAGE_REGISTER( ZMManiMenuUpdate );