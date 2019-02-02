#include "cbase.h"

#include "c_user_message_register.h"

#include "zmr_buildmenu.h"
#include "zmr_zmview_base.h"
#include "zmr/c_zmr_entities.h"
#include "zmr_buildmenu_base.h"


CZMBuildMenuBase::CZMBuildMenuBase( vgui::Panel* pParent, const char* name ) : BaseClass( pParent, name )
{
}

CZMBuildMenuBase::~CZMBuildMenuBase()
{
}

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



	int spawnidx = msg.ReadShort();

    // We don't care about this spawn since we don't have it open...
    if ( spawnidx != pMenu->GetSpawnIndex() ) return;



	bool force_open = msg.ReadOneBit() == 1;


    int count = msg.ReadByte();


	ZMQueueSlotData_t* pQueue = nullptr;

    if ( count > 0 )
    {
        pQueue = new ZMQueueSlotData_t[count];
    }

	for ( int i = 0; i < count; i++ )
	{
		// Every type is increased by 1 so we can send/recv unsigned.
		pQueue[i].zclass = (ZombieClass_t)msg.ReadByte();
		pQueue[i].nCount = msg.ReadByte();
	}

	if ( force_open )
	{
		// If we weren't visible, this is also an opening message
		gViewPortInterface->ShowPanel( pMenu, true );
	}


	pMenu->UpdateQueue( pQueue, count );

    if ( pQueue )
        delete[] pQueue;
}

USER_MESSAGE_REGISTER( ZMBuildMenuUpdate );

