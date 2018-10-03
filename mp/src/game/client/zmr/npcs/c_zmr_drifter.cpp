#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_drifter.h"


#define HALLOWEEN_HAT_MODEL         "models/props/misc/hallo_witchhat.mdl"


IMPLEMENT_CLIENTCLASS_DT( C_ZMDrifter, DT_ZM_Drifter, CZMDrifter )
END_RECV_TABLE()


C_ZMDrifter::C_ZMDrifter()
{
    SetZombieClass( ZMCLASS_DRIFTER );
}

C_ZMDrifter::~C_ZMDrifter()
{

}

bool C_ZMDrifter::IsAffectedByEvent( HappyZombieEvent_t iEvent ) const
{
    return iEvent == HZEVENT_HALLOWEEN;
}

const char* C_ZMDrifter::GetEventHatModel( HappyZombieEvent_t iEvent ) const
{
    return HALLOWEEN_HAT_MODEL;
}
