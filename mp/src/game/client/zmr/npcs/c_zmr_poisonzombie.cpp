#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_poisonzombie.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMPoisonZombie, DT_ZM_PoisonZombie, CNPC_PoisonZombie )
END_RECV_TABLE()


C_ZMPoisonZombie::C_ZMPoisonZombie()
{
    SetZombieClass( ZMCLASS_HULK );
}

C_ZMPoisonZombie::~C_ZMPoisonZombie()
{

}
