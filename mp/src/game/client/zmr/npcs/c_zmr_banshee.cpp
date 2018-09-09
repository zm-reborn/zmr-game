#include "cbase.h"
#include "eventlist.h"

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

void C_ZMBanshee::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_FASTZOMBIE_GALLOP_LEFT )
    {
        if ( ShouldPlayFootstepSound() )
            PlayFootstepSound( "NPC_FastZombie.GallopLeft" );
        return;
    }

    if ( pEvent->event == AE_FASTZOMBIE_GALLOP_RIGHT )
    {
        if ( ShouldPlayFootstepSound() )
            PlayFootstepSound( "NPC_FastZombie.GallopRight" );
        return;
    }
}

void C_ZMBanshee::AttackSound()
{
    EmitSound( "NPC_FastZombie.Attack" );
}
