#include "cbase.h"

#include "zmr_player_resource.h"


IMPLEMENT_SERVERCLASS_ST( CZMPlayerResource, DT_ZM_PlayerResource )
END_SEND_TABLE()

BEGIN_DATADESC( CZMPlayerResource )
END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CZMPlayerResource );
