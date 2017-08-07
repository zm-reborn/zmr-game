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



#include "zmr_manimenu.h"
#include "zmr_buildmenu.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player_shared.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_viewport.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




//using namespace vgui; //don't do this, causes problems with "filesystem" object

CZMBuildMenu *g_pBuildMenu = nullptr;


// ZMRTODO: Remove this old stuff...
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

#define BURNZOMBIE_FLAG 16
#define DRAGZOMBIE_FLAG 8
#define HULK_FLAG 4  
#define FASTIE_FLAG 2
#define SHAMBLIE_FLAG 1

CON_COMMAND( testbuildmenu, "" )
{
    if ( !g_pBuildMenu ) return;


    Msg( "Moveable: %i\n", g_pBuildMenu->IsMoveable() );
}

CZMBuildMenu::CZMBuildMenu( Panel* pParent ) : Frame( g_pClientMode->GetViewport(), "ZMBuildMenu" )
{
	g_pBuildMenu = this;


    SetParent( pParent->GetVPanel() );


    // NOTE: You have to set these before invalidating layout(?)
    SetSizeable( false );
	SetProportional( false );
	SetMoveable( true );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );


	scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ZombieMaster.res", "ZombieMaster" );
	SetScheme( scheme );


	m_hLargeFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet18", true );
	m_hMediumFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet16", true );

	

	LoadControlSettings( "resource/ui/zmbuildmenu.res" );
    InvalidateLayout();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );



	

	//we fetch a bunch of pointers to various elements here so we can alter them quickly and easily
	info_image = dynamic_cast<vgui::ImagePanel*>(FindChildByName("ZombieImage"));

	info_rescost = dynamic_cast<vgui::Label*>(FindChildByName("CostRes"));
	info_popcost = dynamic_cast<vgui::Label*>(FindChildByName("CostPop"));
	info_description = dynamic_cast<vgui::Label*>(FindChildByName("LabelDescription"));

	removelast =  dynamic_cast<vgui::Button*>(FindChildByName("RemoveLast"));
	clearqueue =  dynamic_cast<vgui::Button*>(FindChildByName("ClearQueue"));

	//prepare a list of our spawn buttons etc so we can easily iterate over them
	for (int i=0; i < ZMCLASS_MAX; i++)
	{
		char buffer[25];
		Q_snprintf(buffer, sizeof(buffer), "z_spawn1_%02d", i);
		spawnbuttons[i] = FindChildByName(buffer);

		Q_snprintf(buffer, sizeof(buffer), "z_spawn5_%02d", i);
		spawnfives[i] = FindChildByName(buffer);

		zombieimages[i] = vgui::scheme()->GetImage(TypeToImage[i], true);
		zombiequeue[i] = vgui::scheme()->GetImage(TypeToQueueImage[i], false);
	
	}
	

	KeyValues *kv = new KeyValues("zombiedesc.res");
	if  ( kv->LoadFromFile( (IBaseFileSystem*)filesystem, "resource/zombiedesc.res", "MOD" ) )
	{
		//braaaaaaah, char juggling is pain

		const char *temp = kv->GetString("shambler", "Shambler");
		int length = 128;
		char *saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[ZMCLASS_SHAMBLER] = saved;

		temp = kv->GetString("banshee", "Banshee");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[ZMCLASS_BANSHEE] = saved;

		temp = kv->GetString("hulk", "Hulk");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[ZMCLASS_HULK] = saved;

		temp = kv->GetString("drifter", "Drifter");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[ZMCLASS_DRIFTER] = saved;

		temp = kv->GetString("immolator", "Immolator");
		saved = new char[length];
		Q_strncpy(saved, temp, strlen(temp) + 1);
		zombiedescriptions[ZMCLASS_IMMOLATOR] = saved;
	}
    else
    {
        Warning( "No zombiedesc.res exist!\n" );
    }

	//will delete its child keys as well
	kv->deleteThis();

	for (int i=0; i < BM_QUEUE_SIZE; i++)
	{
		char buffer[10];
		Q_snprintf(buffer, sizeof(buffer), "queue%02d", i);
		queueimages[i] = dynamic_cast<vgui::ImagePanel*>(FindChildByName(buffer));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CZMBuildMenu::~CZMBuildMenu()
{
	for (int i=0; i < TYPE_TOTAL; i++)
	{
		if (zombiedescriptions[i])
			delete zombiedescriptions[i];
	}

}

void CZMBuildMenu::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;

    
    if ( state )
    {
        // Notify the server we've opened this menu.
        engine->ClientCmd( VarArgs( "zm_cmd_openbuildmenu %i", GetSpawnIndex() ) );
    }


    SetVisible( state );
}

void CZMBuildMenu::OnCommand( const char *command )
{
	if ( Q_strnicmp( command, "zm_cmd_queue", 12 ) == 0 )
	{
		char buffer[128];
		Q_snprintf( buffer, sizeof( buffer ), command, GetSpawnIndex() );
		engine->ClientCmd( const_cast<char *>( buffer ) );
	}
	else if ( Q_strnicmp( command, "zm_cmd_queueclear", 17 ) == 0 )
	{
		char buffer[128];
		Q_snprintf( buffer, sizeof( buffer ), command, GetSpawnIndex() );
		engine->ClientCmd( const_cast<char *>( buffer ) );
	}
    else if ( Q_stricmp( command, "createrally" ) == 0 )
    {
        if ( g_pZMView )
            g_pZMView->SetClickMode( ZMCLICKMODE_RALLYPOINT );

        Close();
    }
	else if ( Q_stricmp( command, "vguicancel" ) == 0 )
	{
		Close();
	}

	BaseClass::OnCommand( command );
}

void CZMBuildMenu::OnThink()
{
	if ( !IsVisible() ) return;


    // Make sure we have focus.
    MoveToFront();


	CalculateButtonState();

	for (int i = 0; i < ZMCLASS_MAX; i++)
	{
		if ((spawnbuttons[i] && spawnbuttons[i]->IsCursorOver()) ||
			(spawnfives[i] && spawnfives[i]->IsCursorOver()))
		{
			ShowZombieInfo(i);
		}
	}
}


//TGB: renamed from CalculateFlags to reflect increased functionality
void CZMBuildMenu::CalculateButtonState()
{
    int flags = GetZombieFlags();


	bool button_states[TYPE_TOTAL]; //five buttons

	//TGB: if the flags are 0/unset, all zombies should be available
	//so changed from != 0 to == 0
	if  (flags == 0)
	{
		for (int type=0; type < TYPE_TOTAL; type++)
			button_states[type] = true;
	}
	else
	{
		//Someone's defined ZombieFlags here, so start disabling things
		for (int type=0; type < TYPE_TOTAL; type++)
			button_states[type] = false;

		//Burnzombies
		if (flags & BURNZOMBIE_FLAG)
		{
			button_states[ZMCLASS_IMMOLATOR] = true;
		}
		//Dragzombies
		if (flags & DRAGZOMBIE_FLAG)
		{
			button_states[ZMCLASS_DRIFTER] = true;
		}
		//Hulks
		if (flags & HULK_FLAG)
		{
			button_states[ZMCLASS_HULK] = true;
		}
		//Fasties
		if (flags & FASTIE_FLAG)
		{
			button_states[ZMCLASS_BANSHEE] = true;
		}
		//Shamblies
		if (flags & SHAMBLIE_FLAG)
		{
			button_states[ZMCLASS_SHAMBLER] = true;
		}
	}
		

	for (int type=0; type < TYPE_TOTAL; type++)
	{
		if (spawnbuttons[type])
			spawnbuttons[type]->SetEnabled(button_states[type]);

		if (spawnfives[type])
			spawnfives[type]->SetEnabled(button_states[type]);
			
	}
}

//--------------------------------------------------------------
// TGB: show the zombie img and info in the middle area 
//--------------------------------------------------------------
void CZMBuildMenu::ShowZombieInfo( int type )
{
	if (!info_image || !info_rescost || !info_popcost || !info_description)
		return;

	info_image->SetImage(zombieimages[type]);


    ZombieClass_t zclass = static_cast<ZombieClass_t>( type );

	char buffer[50];
	Q_snprintf(buffer, sizeof(buffer), "%d", C_ZMBaseZombie::GetCost(zclass) );
	info_rescost->SetText(buffer);

	Q_snprintf(buffer, sizeof(buffer), "%d", C_ZMBaseZombie::GetPopCost(zclass));
	info_popcost->SetText(buffer);

	info_description->SetText(zombiedescriptions[type]);
}

//--------------------------------------------------------------
// Update the queue images to reflect the types in the given array 
//--------------------------------------------------------------
void CZMBuildMenu::UpdateQueue( const int q[], int size /*= TYPE_TOTAL*/ )
{
	bool zombies_present = false;
	for ( int i = 0; i < size; i++ )
	{
		const int type = q[i];

		if (!queueimages[i])
			return;

		// Is there a zombie queued at this spot?
		if ( C_ZMBaseZombie::IsValidClass( (ZombieClass_t)type ) )
		{
			vgui::IImage *given_img = zombiequeue[type];

			if (given_img != queueimages[i]->GetImage())
			{
				//queueimages[i]->SetShouldScaleImage(true);
				queueimages[i]->SetImage(given_img);
			}
			queueimages[i]->SetVisible(true);

			zombies_present = true;
		}
		else
		{
			// no valid type, so don't draw an image
			queueimages[i]->SetVisible(false);
		}
		
	}

	if (removelast)
		removelast->SetEnabled(zombies_present);
	if (clearqueue)
		clearqueue->SetEnabled(zombies_present);
	
}

void CZMBuildMenu::OnClose()
{
    // Notify server we've closed this menu.
    engine->ClientCmd( VarArgs( "zm_cmd_closebuildmenu %i", GetSpawnIndex() ) );


    m_iLastSpawnIndex = GetSpawnIndex();
	//SetSpawnIndex( 0 );


	BaseClass::OnClose();
}

//--------------------------------------------------------------
// TGB: usermessage for updating the buildmenu
//--------------------------------------------------------------
void __MsgFunc_ZMBuildMenuUpdate( bf_read &msg )
{
    if ( !g_pBuildMenu ) return;


	//read spawn entindex
	int spawnidx = msg.ReadShort();

    // We don't care about this spawn since we don't have it open...
    if ( spawnidx != g_pBuildMenu->GetSpawnIndex() ) return;



	bool force_open = msg.ReadOneBit() == 1;

	//read queue from usermessage
	int queue[BM_QUEUE_SIZE];
	for ( int i = 0; i < BM_QUEUE_SIZE; i++ )
	{
		//every type was increased by 1 so that type_invalid could be 0 (byte is unsigned)
		queue[i] = ( msg.ReadByte() - 1 );
	}

	if ( force_open )
	{
		//if we weren't visible, this is also an opening message
		gViewPortInterface->ShowPanel( g_pBuildMenu, true );
	}

	//g_pBuildMenu->SetSpawnIndex( spawnidx );
	g_pBuildMenu->UpdateQueue( queue );

}

USER_MESSAGE_REGISTER( ZMBuildMenuUpdate );