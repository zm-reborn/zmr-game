#include "cbase.h"
#include "convar.h"


#include "zmr/zmr_team.h"
#include "zmr/zmr_shareddefs.h"


static ConVar zm_sv_transmitdistance( "zm_sv_transmitdistance", "1600", FCVAR_NOTIFY, "", true, 0.0f, false, 0.0f );

LINK_ENTITY_TO_CLASS( team_manager, CZMTeam );

bool CZMTeam::ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity )
{
    if ( !pRecipient ) return false;

    if ( !pEntity ) return false;



    if (pRecipient->GetTeamNumber() == ZMTEAM_ZM ||
        (pRecipient->IsObserver()
    &&  pRecipient->GetObserverTarget()
    &&  pRecipient->GetObserverTarget()->GetTeamNumber() == ZMTEAM_ZM) )
    {
        // Always transmit the observer target to players
        if ( pRecipient->GetObserverTarget() == pEntity )
            return true;

        // Always transmit players.
        if ( pEntity->IsPlayer() )
            return true;

        // Always transmit zombies.
        if ( pEntity->IsNPC() )
            return true;
        
        float dist = zm_sv_transmitdistance.GetFloat();
        dist *= dist;

        if ( pEntity->GetAbsOrigin().DistToSqr( pRecipient->GetAbsOrigin() ) < dist && UTIL_PointContents( pRecipient->EyePosition() ) & CONTENTS_SOLID )
        {
            return true;
        }
    }


    return false;
}
