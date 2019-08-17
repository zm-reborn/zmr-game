#include "cbase.h"

#include "c_user_message_register.h"

#include "zmr_buildmenu.h"
#include "zmr_zmview_base.h"
#include "zmr/c_zmr_entities.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr/c_zmr_clientmode.h"
#include "zmr_buildmenu_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


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

int CZMBuildMenuBase::ZMKeyInput( ButtonCode_t keynum, int down )
{
    if ( !down )
        return -1;

    int num = -1;


    // Spawn menu only cares about number keys, at least for now

    if ( keynum >= KEY_0 && keynum <= KEY_9 )
        num = keynum - KEY_0;
    if ( keynum >= KEY_PAD_0 && keynum <= KEY_PAD_9 )
        num = keynum - KEY_PAD_0;

    if ( num < 0 )
        return -1;


    
    // Translate the key number to zombie class
    ZombieClass_t zclass = (ZombieClass_t)( num - 1 );

    if ( !C_ZMBaseZombie::IsValidClass( zclass ) )
        return 0;


    bool bDoAltAmount = GetZMClientMode()->IsZMHoldingCtrl();

    // Tell server to spawn em.
    engine->ClientCmd( VarArgs( "zm_cmd_queue %i %i %i",
        GetSpawnIndex(),
        (int)zclass,
        bDoAltAmount ? GetAltSpawnAmount() : 1 ) );
    
    return 1;
}

void CZMBuildMenuBase::ShowMenu( C_ZMEntZombieSpawn* pSpawn )
{
    SetSpawnIndex( pSpawn->entindex() );
    SetZombieFlags( pSpawn->GetZombieFlags() );
    ShowPanel( true );
}

int CZMBuildMenuBase::GetAltSpawnAmount() const
{
    // ZMRTODO: Add cvar or something.
    return 5;
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



	bool active = msg.ReadOneBit() == 1;


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


    // No longer active, so close us.
	if ( !active )
	{
        pMenu->Close();
	}


	pMenu->UpdateQueue( pQueue, count );

    if ( pQueue )
        delete[] pQueue;
}

USER_MESSAGE_REGISTER( ZMBuildMenuUpdate );

