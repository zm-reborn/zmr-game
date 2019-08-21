#include "cbase.h"


#include "zmr/zmr_shareddefs.h"

#include "c_zmr_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



IMPLEMENT_CLIENTCLASS_DT( C_ZMPlayerResource, DT_ZM_PlayerResource, CZMPlayerResource )
END_RECV_TABLE()



C_ZMPlayerResource::C_ZMPlayerResource()
{
    m_Colors[ZMTEAM_HUMAN] = COLOR_RED;
    m_Colors[ZMTEAM_ZM] = COLOR_GREEN;
    m_Colors[ZMTEAM_SPECTATOR] = COLOR_GREY;
    m_Colors[ZMTEAM_UNASSIGNED] = COLOR_WHITE;
}
