#include "cbase.h"

#include "c_zmr_zombiebase.h"
#include "c_zmr_hulk.h"


#define HULKAMANIA_MODEL        "models/props/misc/hulkamania01.mdl"


IMPLEMENT_CLIENTCLASS_DT( C_ZMHulk, DT_ZM_Hulk, CZMHulk )
END_RECV_TABLE()


C_ZMHulk::C_ZMHulk()
{
    SetZombieClass( ZMCLASS_HULK );
}

C_ZMHulk::~C_ZMHulk()
{

}

void C_ZMHulk::FootstepSound( bool bRightFoot )
{
    EmitSound( bRightFoot ? "NPC_PoisonZombie.FootstepRight" : "NPC_PoisonZombie.FootstepLeft" );
}

void C_ZMHulk::AttackSound()
{
    EmitSound( "NPC_PoisonZombie.Attack" );
}

bool C_ZMHulk::IsAffectedByEvent( HappyZombieEvent_t iEvent ) const
{
    return iEvent == HZEVENT_HULKAMANIA;
}

const char* C_ZMHulk::GetEventHatModel( HappyZombieEvent_t iEvent ) const
{
    return HULKAMANIA_MODEL;
}
