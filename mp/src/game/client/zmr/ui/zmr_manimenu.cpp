#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>

#include "c_user_message_register.h"

#include "iclientmode.h"


#include "zmr/c_zmr_entities.h"
#include "zmr_manimenu.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player_shared.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_viewport.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




using namespace vgui;

CZMManiMenu *g_pManiMenu = nullptr;

CZMManiMenu::CZMManiMenu( Panel* pParent ) : Frame( g_pClientMode->GetViewport(), "ZMManiMenu" )
{
	g_pManiMenu = this;

	//m_pViewPort = pViewPort;

    SetParent( pParent->GetVPanel() );


    SetTrapIndex( 0 );

    // NOTE: You have to set these before invalidating layout(?)
    SetPaintBackgroundEnabled( false );
    SetSizeable( false );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );
    SetProportional( false );
    SetMoveable( true );
    


	scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ZombieMaster.res", "ZombieMaster" );
	SetScheme( scheme );


	m_hLargeFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Trebuchet18", true );
	m_hMediumFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Trebuchet16", true );

	

	LoadControlSettings( "resource/ui/zmmanimenunew.res" );
    InvalidateLayout();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );


    Button* button = dynamic_cast<Button*>( FindChildByName( "Description" ) );
    if ( button ) button->SetAsDefaultButton( 1 );
}

CZMManiMenu::~CZMManiMenu()
{

}

void CZMManiMenu::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;


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
    Q_snprintf( buffer, sizeof( buffer ), "Activate %i.",  cost );

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
        Q_snprintf( buffer, sizeof( buffer ), "Trap for %i.",  cost );
    }
    else
    {
        // Are traps gay?
        Q_snprintf( buffer, sizeof( buffer ), "Trap disabled." );
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

    Vector screen;
    int x, y;
    if ( CZMFrame::WorldToScreen( GetTrapPos(), screen, x, y ) )
    {
        int w, h;
        GetSize( w, h );


        x -= w / 2.0f;
        y -= h / 1.5f;


        if ( x < 0 ) x = 0;
        if ( y < 0 ) y = 0;
        if ( (x+w) >= ScreenWidth() ) x = ScreenWidth() - w;
        if ( (y+h) >= ScreenHeight() ) y = ScreenHeight() - h;

        SetPos( x, y );
    }
}

void __MsgFunc_ZMManiMenuUpdate( bf_read &msg )
{
    if ( !g_pManiMenu ) return;


    // The button will allocate memory.
    char desc[256];
    msg.ReadString( desc, sizeof( desc ), true );

    int index = g_pManiMenu->GetTrapIndex();

    if ( index > 0 )
    {
        // ZMRTODO: See if this is even the right way to do it, lol. I mean, the edict index is usually the same, so in that department it's passable but is there an easier method?
        IHandleEntity* pHandle = cl_entitylist->LookupEntityByNetworkIndex( index );

        if ( pHandle )
        {
            C_ZMEntManipulate* pMani = dynamic_cast<C_ZMEntManipulate*>( EntityFromEntityHandle( pHandle ) );

            if ( pMani )
                pMani->SetDescription( desc );
        }
    }


    g_pManiMenu->SetDescription( desc );
    
}

USER_MESSAGE_REGISTER( ZMManiMenuUpdate );