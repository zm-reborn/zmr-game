#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_banshee.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMBanshee, DT_ZM_Banshee, CZMBanshee )
END_RECV_TABLE()


C_ZMBanshee::C_ZMBanshee()
{
    SetZombieClass( ZMCLASS_BANSHEE );
}

C_ZMBanshee::~C_ZMBanshee()
{

}
