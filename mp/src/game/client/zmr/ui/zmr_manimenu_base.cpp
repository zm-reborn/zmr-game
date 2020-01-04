#include "cbase.h"

#include "c_user_message_register.h"

#include "zmr_zmview_base.h"
#include "zmr/c_zmr_entities.h"
#include "zmr_manimenu_base.h"



void CZMManiMenuBase::OnCommand( const char *command )
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

void CZMManiMenuBase::ShowMenu( C_ZMEntManipulate* pMani )
{
    SetTrapIndex( pMani->entindex() );
    SetDescription( *pMani->GetDescription() ? pMani->GetDescription() : "Activate trap." );
    SetCost( pMani->GetCost() );
    SetTrapCost( pMani->GetTrapCost() );
	pMani->SetMenu( this );

    ShowPanel( true );
}

void CZMManiMenuBase::UpdateMenu( C_ZMEntManipulate* pMani )
{
    SetDescription( *pMani->GetDescription() ? pMani->GetDescription() : "Activate trap." );
    SetCost( pMani->GetCost() );
    SetTrapCost( pMani->GetTrapCost() );
}

void CZMManiMenuBase::OnClose()
{
    // Notify server we've closed this menu.
    engine->ClientCmd( VarArgs( "zm_cmd_closemenu %i", GetTrapIndex() ) );

	BaseClass::OnClose();
}

void __MsgFunc_ZMManiMenuUpdate( bf_read &msg )
{
    if ( !g_pZMView || !g_pZMView->GetManiMenu() ) return;


    CZMManiMenuBase* pMenu = g_pZMView->GetManiMenu();



	int spawnidx = msg.ReadShort();

    // We don't care about this trap since we don't have it open...
    if ( spawnidx != pMenu->GetTrapIndex() ) return;


	bool active = msg.ReadOneBit() == 1;


    // No longer active, so close us.
	if ( !active )
	{
        pMenu->Close();
	}
}

USER_MESSAGE_REGISTER( ZMManiMenuUpdate );
