#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_immolator.h"


IMPLEMENT_CLIENTCLASS_DT( C_ZMImmolator, DT_ZM_Immolator, CZMImmolator )
END_RECV_TABLE()


C_ZMImmolator::C_ZMImmolator()
{
    SetZombieClass( ZMCLASS_IMMOLATOR );
}

C_ZMImmolator::~C_ZMImmolator()
{

}
