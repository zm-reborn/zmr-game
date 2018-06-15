#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_drifter.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMDrifter, DT_ZM_Drifter, CZMDrifter )
END_RECV_TABLE()


C_ZMDrifter::C_ZMDrifter()
{
    SetZombieClass( ZMCLASS_DRIFTER );
}

C_ZMDrifter::~C_ZMDrifter()
{

}
