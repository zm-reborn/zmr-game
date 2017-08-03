#include "cbase.h"

#include "c_zmr_entities.h"
#include "zmr/zmr_player_shared.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMEntBaseUsable, DT_ZM_EntBaseUsable, CZMEntBaseUsable )
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntBaseUsable )
END_DATADESC()

/*bool C_ZMEntBaseUsable::ShouldDraw()
{
    C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( pPlayer && pPlayer->IsHuman() ) return false;


    return BaseClass::ShouldDraw();
}*/

int C_ZMEntBaseUsable::DrawModel( int flags )
{
    C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( !pPlayer || pPlayer->IsHuman() ) return 0;


    return BaseClass::DrawModel( flags );
}


IMPLEMENT_CLIENTCLASS_DT( C_ZMEntBaseSimple, DT_ZM_EntBaseSimple, CZMEntBaseSimple )
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntBaseSimple )
END_DATADESC()

int C_ZMEntBaseSimple::DrawModel( int flags )
{
    C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );

    if ( !pPlayer || pPlayer->IsHuman() ) return 0;


    return BaseClass::DrawModel( flags );
}


IMPLEMENT_CLIENTCLASS_DT( C_ZMEntZombieSpawn, DT_ZM_EntZombieSpawn, CZMEntZombieSpawn )
    RecvPropInt( RECVINFO( m_fZombieFlags ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntZombieSpawn )
END_DATADESC()


IMPLEMENT_CLIENTCLASS_DT( C_ZMEntManipulate, DT_ZM_EntManipulate, CZMEntManipulate )
    RecvPropInt( RECVINFO( m_nCost ) ),
    RecvPropInt( RECVINFO( m_nTrapCost ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntManipulate )
END_DATADESC()


C_ZMEntManipulate::C_ZMEntManipulate()
{

}