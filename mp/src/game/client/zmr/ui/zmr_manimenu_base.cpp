#include "cbase.h"

#include "c_user_message_register.h"

#include "zmr_viewport.h"
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
    ShowPanel( true );
}

void __MsgFunc_ZMManiMenuUpdate( bf_read &msg )
{
    if ( !g_pZMView || !g_pZMView->GetManiMenu() ) return;


    CZMManiMenuBase* pMenu = g_pZMView->GetManiMenu();

    // The button will allocate memory.
    char desc[256];
    msg.ReadString( desc, sizeof( desc ), true );

    int index = pMenu->GetTrapIndex();

    if ( index > 0 )
    {
        IHandleEntity* pHandle = cl_entitylist->LookupEntityByNetworkIndex( index );

        if ( pHandle )
        {
            C_ZMEntManipulate* pMani = dynamic_cast<C_ZMEntManipulate*>( EntityFromEntityHandle( pHandle ) );

            if ( pMani )
                pMani->SetDescription( desc );
        }
    }

    pMenu->SetDescription( desc );
}

USER_MESSAGE_REGISTER( ZMManiMenuUpdate );
