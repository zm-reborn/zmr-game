#include "cbase.h"

#ifdef GAME_DLL
#include "zmr_base.h"
#include "zmr_usercmdvalid.h"


CZMUserCmdHitValidator::CZMUserCmdHitValidator()
{
    m_pszUserCmdError = "";
}

bool CZMUserCmdHitWepValidator::IsUserCmdHitsValid( ZMUserCmdValidData_t& data )
{
    Assert( data.pAttacker != nullptr && data.pVictim != nullptr );


    if ( !g_ZMUserCmdSystem.UsesClientsideDetection( data.pVictim ) )
        return OnUserCmdError( "Victim does not use clientside detection!" );


    int maxBullets = GetMaxUserCmdBullets( data );
    if ( maxBullets > 0 && data.nHits > maxBullets )
    {
        return OnUserCmdError( "Entity had more hits than bullets fired!" );
    }


    int maxPenetrate = -1;
    if ( maxBullets > 0 )
        maxPenetrate = (GetMaxNumPenetrate( data ) + 1) * maxBullets;

    if ( maxPenetrate > 0 && maxPenetrate < data.nAlreadyHit )
    {
        return OnUserCmdError( "Usercmd has too many penetrations!" );
    }


    Vector dir = data.pVictim->WorldSpaceCenter() - data.vecSrc;
    float dist = dir.NormalizeInPlace();

    float maxdist = GetMaxDamageDist( data );
    if ( maxdist > 0.0f && dist > maxdist )
    {
        return OnUserCmdError( "Player is too far to hit!" );
    }


    return true;
}
#endif
