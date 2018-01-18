#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_fastzombie.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMFastZombie, DT_ZM_FastZombie, CFastZombie )
END_RECV_TABLE()


C_ZMFastZombie::C_ZMFastZombie()
{
    SetZombieClass( ZMCLASS_BANSHEE );
}

C_ZMFastZombie::~C_ZMFastZombie()
{

}
