#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_burnzombie.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMBurnZombie, DT_ZM_BurnZombie, CNPC_BurnZombie )
END_RECV_TABLE()


C_ZMBurnZombie::C_ZMBurnZombie()
{
    SetZombieClass( ZMCLASS_IMMOLATOR );
}

C_ZMBurnZombie::~C_ZMBurnZombie()
{

}
