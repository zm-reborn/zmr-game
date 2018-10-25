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
    ShowPanel( true );
}
