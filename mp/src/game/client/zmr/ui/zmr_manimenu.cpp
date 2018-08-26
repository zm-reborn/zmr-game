#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>

#include "iclientmode.h"


#include "zmr_manimenu.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player_shared.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr/c_zmr_util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




using namespace vgui;


CZMManiMenu::CZMManiMenu( Panel* pParent ) : CZMManiMenuBase( "ZMManiMenu" )
{
    SetParent( pParent->GetVPanel() );


    SetTrapIndex( 0 );


    SetSizeable( false );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );
    SetProportional( false );
    SetMoveable( true );


	SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ZombieMaster.res", "ZombieMaster" ) );

	LoadControlSettings( "resource/ui/zmmanimenu.res" );


    vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );
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
    Q_snprintf( buffer, sizeof( buffer ), "Activate for %i.",  cost );

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

void CZMManiMenu::OnThink()
{
	if ( !IsVisible() ) return;


    MoveToFront();


	C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
	{
        return;
	}


    SetControlEnabled( "Activate", ( pPlayer->GetResources() >= m_nCost ) );
    SetControlEnabled( "Trap", ( m_nTrapCost > 0 && pPlayer->GetResources() >= m_nTrapCost ) );
}
