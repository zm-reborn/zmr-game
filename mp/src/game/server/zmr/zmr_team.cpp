#include "cbase.h"
#include "zmr/zmr_team.h"
#include "zmr/zmr_shareddefs.h"


LINK_ENTITY_TO_CLASS( team_manager, CZMTeam );

bool CZMTeam::ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity )
{
    if ( !pRecipient ) return false;

    // Always transmit to ZM.
    if ( pRecipient->GetTeamNumber() == ZMTEAM_ZM )
    {
        //if ( UTIL_PointContents( pRecipient->EyePosition() ) & CONTENTS_SOLID )
        //{
            return true;
        //}
    }
    
    if ( pRecipient->IsObserver() && pRecipient->GetObserverTarget() )
    {
        // Always transmit the observer target to players
        if ( pRecipient->GetObserverTarget() == pEntity )
            return true;


        if ( pRecipient->GetObserverTarget()->IsPlayer() )
        {
            // If the our observer target is the ZM, always show other players as well.
            if ( pRecipient->GetObserverTarget()->GetTeamNumber() == ZMTEAM_ZM )
                return true;
        }
    }

    return false;
}
