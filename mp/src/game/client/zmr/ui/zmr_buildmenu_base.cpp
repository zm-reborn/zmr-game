#include "cbase.h"

#include "c_user_message_register.h"

#include "zmr_buildmenu.h"
#include "zmr_zmview_base.h"
#include "zmr/c_zmr_entities.h"
#include "zmr_buildmenu_base.h"


void CZMBuildMenuBase::OnCommand( const char *command )
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

void CZMBuildMenuBase::ShowMenu( C_ZMEntZombieSpawn* pSpawn )
{
    SetSpawnIndex( pSpawn->entindex() );
    SetZombieFlags( pSpawn->GetZombieFlags() );
    ShowPanel( true );
}

void CZMBuildMenuBase::OnClose()
{
    // Notify server we've closed this menu.
    engine->ClientCmd( VarArgs( "zm_cmd_closebuildmenu %i", GetSpawnIndex() ) );

    m_iLastSpawnIndex = GetSpawnIndex();


	BaseClass::OnClose();
}

void __MsgFunc_ZMBuildMenuUpdate( bf_read &msg )
{
    if ( !g_pZMView || !g_pZMView->GetBuildMenu() ) return;


    CZMBuildMenuBase* pMenu = g_pZMView->GetBuildMenu();


	//read spawn entindex
	int spawnidx = msg.ReadShort();

    // We don't care about this spawn since we don't have it open...
    if ( spawnidx != pMenu->GetSpawnIndex() ) return;



	bool force_open = msg.ReadOneBit() == 1;

	//read queue from usermessage
	int queue[BM_QUEUE_SIZE];
	for ( int i = 0; i < ARRAYSIZE( queue ); i++ )
	{
		//every type was increased by 1 so that type_invalid could be 0 (byte is unsigned)
		queue[i] = ( msg.ReadByte() - 1 );
	}

	if ( force_open )
	{
		//if we weren't visible, this is also an opening message
		gViewPortInterface->ShowPanel( pMenu, true );
	}


	pMenu->UpdateQueue( queue, ARRAYSIZE( queue ) );
}

USER_MESSAGE_REGISTER( ZMBuildMenuUpdate );

