#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_dragzombie.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMDragZombie, DT_ZM_DragZombie, CNPC_DragZombie )
END_RECV_TABLE()


C_ZMDragZombie::C_ZMDragZombie()
{
    SetZombieClass( ZMCLASS_DRIFTER );
}

C_ZMDragZombie::~C_ZMDragZombie()
{

}
