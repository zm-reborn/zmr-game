#include "cbase.h"

#include "c_zmr_playeranimevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
    RecvPropEHandle( RECVINFO( m_hPlayer ) ),
    RecvPropInt( RECVINFO( m_iEvent ) ),
    RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()