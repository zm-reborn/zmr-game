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


#include "zmr_manimenu.h"
#include "zmr_buildmenunew.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player_shared.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_viewport.h"
#include "zmr_radial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




using namespace vgui;

CZMBuildMenuNew *g_pBuildMenuNew = nullptr;


// ZMRTODO: Remove this old stuff...
/*
const char *TypeToImage[ZMCLASS_MAX] = {
		"zombies/info_shambler",
		"zombies/info_banshee",
		"zombies/info_hulk",
		"zombies/info_drifter",
		"zombies/info_immolator"
};

const char *TypeToQueueImage[ZMCLASS_MAX] = {
		"zombies/queue_shambler",
		"zombies/queue_banshee",
		"zombies/queue_hulk",
		"zombies/queue_drifter",
		"zombies/queue_immolator",
};
*/

const char* g_QueueImage[ZMCLASS_MAX] = {
		"zombies/queue_shambler",
		"zombies/queue_banshee",
		"zombies/queue_hulk",
		"zombies/queue_drifter",
		"zombies/queue_immolator",
};

CZMBuildMenuNew::CZMBuildMenuNew( Panel* pParent ) : CZMWorldPanel( "ZMBuildMenuNew" )
{
	g_pBuildMenuNew = this;


    SetParent( pParent->GetVPanel() );


    // NOTE: You have to set these before invalidating layout(?)
    SetPaintBackgroundEnabled( false );
    SetSizeable( false );
	SetProportional( false );
	SetMoveable( true );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );


	SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );

	LoadControlSettings( "resource/ui/zmbuildmenunew.res" );
    //InvalidateLayout();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );


    ZMRadialPanel* radial = dynamic_cast<ZMRadialPanel*>( FindChildByName( "ZMRadialPanel1" ) );

    if ( radial )
    {
        radial->LoadFromFile( "resource/zmradial_spawn.txt" );
        radial->SetBackgroundImage( "zmr_buildmenu/bg" );
    }


    for ( int i = 0; i < ZMCLASS_MAX; i++ )
    {
        m_pQueueImages[i] = vgui::scheme()->GetImage( g_QueueImage[i], false );
    }

    for ( int i = 0; i < BM_QUEUE_SIZE; i++ )
    {
        char buffer[32];
        Q_snprintf( buffer, sizeof( buffer ), "queue%02i", i );
        m_pQueue[i] = dynamic_cast<ImagePanel*>( FindChildByName( buffer ) );
    }

    SetOffset( 160 + 128, 0 + 128 );
    SetLimits( 160, 0, 160 + 256, 256 );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CZMBuildMenuNew::~CZMBuildMenuNew()
{
}

void CZMBuildMenuNew::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;

    
    if ( state )
    {
        // Notify the server we've opened this menu.
        engine->ClientCmd( VarArgs( "zm_cmd_openbuildmenu %i", GetSpawnIndex() ) );
    }


    SetVisible( state );
}

void CZMBuildMenuNew::OnCommand( const char *command )
{
	if ( Q_strnicmp( command, "zm_cmd_queue", 12 ) == 0 )
	{
		char buffer[128];
		Q_snprintf( buffer, sizeof( buffer ), command, GetSpawnIndex() );
		engine->ClientCmd( const_cast<char *>( buffer ) );
        return;
	}
	else if ( Q_strnicmp( command, "zm_cmd_queueclear", 17 ) == 0 )
	{
		char buffer[128];
		Q_snprintf( buffer, sizeof( buffer ), command, GetSpawnIndex() );
		engine->ClientCmd( const_cast<char *>( buffer ) );
        return;
	}
    else if ( Q_stricmp( command, "createrally" ) == 0 )
    {
        if ( g_pZMView )
            g_pZMView->SetClickMode( ZMCLICKMODE_RALLYPOINT );

        Close();
        return;
    }
    else if ( Q_stricmp( command, "Close" ) == 0 )
    {
        Close();
        return;
    }
    
	BaseClass::OnCommand( command );
}

void CZMBuildMenuNew::OnThink()
{
	if ( !IsVisible() ) return;


    // Make sure we have focus.
    MoveToFront();


    BaseClass::OnThink();
}

void CZMBuildMenuNew::OnClose()
{
    // Notify server we've closed this menu.
    engine->ClientCmd( VarArgs( "zm_cmd_closebuildmenu %i", GetSpawnIndex() ) );


    m_iLastSpawnIndex = GetSpawnIndex();
	//SetSpawnIndex( 0 );


	//BaseClass::OnClose();
}

void CZMBuildMenuNew::UpdateQueue( const ZombieClass_t queue[] )
{
    bool anyinqueue = false;


    for ( int i = 0; i < BM_QUEUE_SIZE; i++ )
    {
		const ZombieClass_t type = queue[i];

		if ( !m_pQueue[i] ) continue;


		// Is there a zombie queued at this spot?
		if ( C_ZMBaseZombie::IsValidClass( type ) )
		{
			vgui::IImage* type_img = m_pQueueImages[(int)type];

			if ( type_img && type_img != m_pQueue[i]->GetImage() )
			{
				m_pQueue[i]->SetImage( type_img );
			}

			m_pQueue[i]->SetVisible( true );

			anyinqueue = true;
		}
		else
		{
			m_pQueue[i]->SetVisible( false );
		}
    }

	//if ( removelast )
	//	removelast->SetEnabled( anyinqueue );

	//if ( clearqueue )
	//	clearqueue->SetEnabled( anyinqueue );
}

void __MsgFunc_ZMBuildMenuUpdate( bf_read &msg )
{
    if ( !g_pBuildMenuNew ) return;


	//read spawn entindex
	int spawnidx = msg.ReadShort();

    // We don't care about this spawn since we don't have it open...
    if ( spawnidx != g_pBuildMenuNew->GetSpawnIndex() ) return;



	bool force_open = msg.ReadOneBit() == 1;

	//read queue from usermessage
	ZombieClass_t queue[BM_QUEUE_SIZE];
	for ( int i = 0; i < BM_QUEUE_SIZE; i++ )
	{
		//every type was increased by 1 so that type_invalid could be 0 (byte is unsigned)
		queue[i] = (ZombieClass_t)(msg.ReadByte() - 1);
	}

	if ( force_open )
	{
		//if we weren't visible, this is also an opening message
		gViewPortInterface->ShowPanel( g_pBuildMenuNew, true );
	}

	g_pBuildMenuNew->UpdateQueue( queue );
}

USER_MESSAGE_REGISTER( ZMBuildMenuUpdate );
