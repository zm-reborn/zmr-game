#include "cbase.h"

#include "zmr_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_SERVERCLASS_ST( CZMPlayerResource, DT_ZM_PlayerResource )
END_SEND_TABLE()

BEGIN_DATADESC( CZMPlayerResource )
END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CZMPlayerResource );
