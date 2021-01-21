#include "cbase.h"
#include "takedamageinfo.h"
#include "soundent.h"

#include "npcr/npcr_basenpc.h"

#include "zmr_zombie_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CZMZombieSenses::CZMZombieSenses( NPCR::CBaseNPC* pNPC ) : NPCR::CBaseSenses( pNPC )
{

}

CZMZombieSenses::~CZMZombieSenses()
{
}

void CZMZombieSenses::OnDamaged( const CTakeDamageInfo& info )
{
    // Give the zombie free knowledge of our attacker.

    CBaseEntity* pAttacker = info.GetAttacker();

    if ( pAttacker && GetNPC()->IsEnemy( pAttacker ) )
    {
        auto* pKnown = FindKnownOf( pAttacker );

        if ( !pKnown )
        {
            pKnown = new NPCR::KnownEntity( pAttacker, false );
            

            m_vKnownEnts.AddToTail( pKnown );
            
            GetNPC()->OnAcquiredEnemy( pAttacker );
        }

        pKnown->UpdateLastSensed();
    }
}
