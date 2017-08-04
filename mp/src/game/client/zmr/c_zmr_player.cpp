#include "cbase.h"

#include "baseviewport.h"


#include "zmr/ui/zmr_viewport.h"

#include "npcs/c_zmr_zombiebase.h"
#include "zmr/zmr_global_shared.h"

#include "c_zmr_player.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMPlayer, DT_ZM_Player, CZMPlayer )
    RecvPropInt( RECVINFO( m_nResources ) ),
END_RECV_TABLE()

//BEGIN_PREDICTION_DATA( C_ZMPlayer )
//END_PREDICTION_DATA();

void C_ZMPlayer::TeamChange( int iNewTeam )
{
    BaseClass::TeamChange( iNewTeam );


    if ( g_pZMView )
        g_pZMView->SetVisible( iNewTeam == ZMTEAM_ZM );
}