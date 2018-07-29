#include "cbase.h"

#include "c_zmr_zombieanimevent.h"


IMPLEMENT_CLIENTCLASS_EVENT( C_ZMTEZombieAnimEvent, DT_ZM_TEZombieAnimEvent, CZMTEZombieAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_ZMTEZombieAnimEvent, DT_ZM_TEZombieAnimEvent )
    RecvPropEHandle( RECVINFO( m_hZombie ) ),
    RecvPropInt( RECVINFO( m_iEvent ) ),
    //RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()
