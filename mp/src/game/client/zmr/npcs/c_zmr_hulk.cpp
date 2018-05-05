#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_hulk.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMHulk, DT_ZM_Hulk, CZMHulk )
END_RECV_TABLE()


C_ZMHulk::C_ZMHulk()
{
    SetZombieClass( ZMCLASS_HULK );
}

C_ZMHulk::~C_ZMHulk()
{

}
