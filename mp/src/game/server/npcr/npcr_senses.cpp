#include "cbase.h"
#include "soundent.h"

#include "npcr_nonplayer.h"
#include "npcr_senses.h"



#define VISION_LASTSEEN_GRACE   0.2f
#define KNOWN_FORGET_TIME       6.0f

ConVar npcr_debug_senses( "npcr_debug_senses", "0" );


class CVisionFilter : public CTraceFilterSimple
{
public:
    DECLARE_CLASS( CVisionFilter, CTraceFilterSimple );

    CVisionFilter( NPCR::CBaseNPC* pNPC ) : BaseClass( pNPC->GetCharacter(), COLLISION_GROUP_NONE, nullptr )
    {
        m_pSenses = pNPC->GetSenses();
    }

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE
    {
        bool ret = BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
        if ( !ret )
            return false;


        auto* pEnt = EntityFromEntityHandle( pHandleEntity );
	    if ( !pEnt )
		    return false;


        return !m_pSenses->CanSeeThrough( pEnt );
    }

private:
    NPCR::CBaseSenses* m_pSenses;
};


NPCR::CBaseSenses::CBaseSenses( NPCR::CBaseNPC* pNPC ) : NPCR::CEventListener( pNPC, pNPC )
{
}

NPCR::CBaseSenses::~CBaseSenses()
{
    m_vKnownEnts.PurgeAndDeleteElements();
}

void NPCR::CBaseSenses::Update()
{
    if ( ShouldUpdateVision() )
        UpdateVision();

    if ( ShouldUpdateHearing() )
        UpdateHearing();


    if ( IsDebugging() )
    {
        float interval = GetNPC()->GetUpdateInterval();
        char buffer[256];
        float curtime = gpGlobals->curtime;

        FOR_EACH_VEC( m_vKnownEnts, i )
        {
            auto& known = m_vKnownEnts[i];

            auto* pEnt = known->GetEntity();
            if ( !pEnt ) continue;

            auto sensed = curtime - known->LastSensedTime();
            auto seen = curtime - known->LastSeenTime();
            Q_snprintf( buffer, sizeof( buffer ), "Sensed: %.1f | Seen: %.1f", sensed, seen );
            NDebugOverlay::Text( pEnt->GetAbsOrigin(), buffer, true, interval );
        }
        
    }
}

bool NPCR::CBaseSenses::ShouldUpdateVision()
{
    return m_NextVisionTimer.IsElapsed();
}

bool NPCR::CBaseSenses::ShouldUpdateHearing()
{
    return m_NextHearingTimer.IsElapsed();
}

void NPCR::CBaseSenses::UpdateHearing()
{
    if ( 1 /*&& !(GetOuter()->HasSpawnFlags(SF_NPC_WAIT_TILL_SEEN))*/ )
    {
        const int iSoundMask = GetSoundMask();
    
        int	iSound = CSoundEnt::ActiveList();
        while ( iSound != SOUNDLIST_EMPTY )
        {
            CSound* pSound = CSoundEnt::SoundPointerForIndex( iSound );

            if ( !pSound )
                break;

            if ( (iSoundMask & pSound->SoundType()) )
            {
                if ( CanHearSound( pSound ) )
                {
                    // the npc cares about this sound, and it's close enough to hear.
                    //pCurrentSound->m_iNextAudible = m_iAudibleList;
                    //m_iAudibleList = iSound;
                    GetNPC()->OnHeardSound( pSound );
                }
            }

            iSound = pSound->NextSound();
        }
    }

    m_NextHearingTimer.Start( 0.1f );
}

int NPCR::CBaseSenses::GetSoundMask() const
{
    return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_BULLET_IMPACT;
}

unsigned int NPCR::CBaseSenses::GetVisionMask() const
{
    return MASK_VISIBLE | CONTENTS_BLOCKLOS | CONTENTS_MONSTER;
}

bool NPCR::CBaseSenses::CanSeeThrough( CBaseEntity* pEnt ) const
{
    return !pEnt->BlocksLOS();
}

bool NPCR::CBaseSenses::CanHearSound( CSound* pSound ) const
{
    if ( pSound->Volume() <= 0.0f )
        return false;


    float flHearingDist = pSound->Volume();
    flHearingDist *= flHearingDist;

    return pSound->GetSoundOrigin().DistToSqr( GetNPC()->GetPosition() ) < flHearingDist;
}

void NPCR::CBaseSenses::UpdateVision()
{
    m_flFovCos = cosf( DEG2RAD( GetFieldOfView() / 2.0f ) );


    //
    // Update all the known entities.
    //
    for ( int i = m_vKnownEnts.Count() - 1; i >= 0; i-- )
    {
        auto* pKnown = m_vKnownEnts[i];
        CBaseEntity* pLost = pKnown->GetEntity();

        bool bValid = IsValidKnownEntity( pLost );

        // Update vision
        if ( bValid )
        {
            UpdateVisionForKnown( pKnown );
        }

        // Remove forgettable entities.
        if ( !bValid || ShouldForgetEntity( pKnown ) )
        {
            if ( bValid )
                GetNPC()->OnLostEnemy( pLost );

            m_vKnownEnts.Remove( i );
            delete pKnown;
        }
    }
    
    //
    // Find new entities.
    //
    CUtlVector<CBaseEntity*> vListEnts;
    FindNewEntities( vListEnts );

    FOR_EACH_VEC( vListEnts, i )
    {
        // We already know about this entity.
        if ( GetKnownOf( vListEnts[i] ) )
            continue;


        if ( !CanSeeCharacter( vListEnts[i] ) )
            continue;


        m_vKnownEnts.AddToTail( new KnownEntity( vListEnts[i], true ) );

        GetNPC()->OnAcquiredEnemy( vListEnts[i] );
    }

    m_NextVisionTimer.Start( 0.1f );
}

void NPCR::CBaseSenses::FindNewEntities( CUtlVector<CBaseEntity*>& vListEnts )
{
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );
        if ( pPlayer && GetNPC()->IsEnemy( pPlayer ) )
        {
            vListEnts.AddToTail( pPlayer );
        }
    }
}

const NPCR::KnownEntity* NPCR::CBaseSenses::GetKnownOf( CBaseEntity* pEnt ) const
{
    return FindKnownOf( pEnt );
}

NPCR::KnownEntity* NPCR::CBaseSenses::FindKnownOf( CBaseEntity* pEnt ) const
{
    if ( !pEnt ) return nullptr;


    FOR_EACH_VEC( m_vKnownEnts, i )
    {
        if ( m_vKnownEnts[i]->GetEntity() == pEnt )
            return m_vKnownEnts[i];
    }

    return nullptr;
}

bool NPCR::CBaseSenses::ShouldForgetEntity( const KnownEntity* pKnown ) const
{
    return gpGlobals->curtime - pKnown->LastSensedTime() > KNOWN_FORGET_TIME;
}

void NPCR::CBaseSenses::UpdateVisionForKnown( KnownEntity* pKnown )
{
    auto* pEnt = pKnown->GetEntity();
    if ( !pEnt )
        return;

    if ( (gpGlobals->curtime - pKnown->LastSeenTime()) < VISION_LASTSEEN_GRACE )
        return;


    bool bDidSee = pKnown->CanSee();
    bool bCanSee = CanSeeCharacter( pEnt );


    if ( bCanSee )
    {
        if ( !bDidSee )
            GetNPC()->OnSightGained( pEnt );

        pKnown->UpdateLastSeen();
    }
    else
    {
        if ( bDidSee )
            GetNPC()->OnSightLost( pEnt );

        pKnown->SetCantSee();
    }
}

bool NPCR::CBaseSenses::CanSee( CBaseEntity* pEnt ) const
{
    FOR_EACH_VEC( m_vKnownEnts, i )
    {
        if ( m_vKnownEnts[i]->GetEntity() == pEnt )
            return CanSee( m_vKnownEnts[i] );
    }

    return false;
}

bool NPCR::CBaseSenses::CanSee( KnownEntity* pKnown ) const
{
    return pKnown->CanSee();
}

bool NPCR::CBaseSenses::CanSee( const Vector& vecPos ) const
{
    CBaseCombatCharacter* pChar = GetOuter();

    Vector fwd;
    Vector start = pChar->EyePosition();
    Vector dir = vecPos - start;

    // Is the position within our field of view?
    AngleVectors( GetNPC()->GetAngles(), &fwd );
    Vector dir_noz = Vector( dir.x, dir.y, 0.0f ).Normalized();
    float dot = dir_noz.Dot( fwd );

    if ( dot < m_flFovCos )
    {
        return false;
    }

    if ( dir.NormalizeInPlace() > GetVisionDistance() )
    {
        return false;
    }


    return HasLOS( vecPos );
}

bool NPCR::CBaseSenses::HasLOS( const Vector& vecPos ) const
{
    trace_t tr;
    CVisionFilter filter( GetNPC() );

    UTIL_TraceLine( GetOuter()->EyePosition(), vecPos, GetVisionMask(), &filter, &tr );
    
    return tr.fraction == 1.0f;
}

CBaseEntity* NPCR::CBaseSenses::GetClosestEntity() const
{
    float closest = FLT_MAX;
    const Vector vecMyPos = GetOuter()->WorldSpaceCenter();
    CBaseEntity* pClosest = nullptr;

    int len = m_vKnownEnts.Count();
    for ( int i = 0; i < len; i++ )
    {
        CBaseEntity* pEnt = m_vKnownEnts[i]->GetEntity();
        if ( !pEnt ) continue;


        float dist = pEnt->WorldSpaceCenter().DistToSqr( vecMyPos );
        if ( dist < closest )
        {
            closest = dist;
            pClosest = pEnt;
        }
    }

    return pClosest;
}

bool NPCR::CBaseSenses::IsValidKnownEntity( CBaseEntity* pEnt ) const
{
    return pEnt && GetNPC()->IsEnemy( pEnt );
}

bool NPCR::CBaseSenses::CanSeeCharacter( CBaseEntity* pEnt ) const
{
    // Check plausible spots
    Vector checkSpots[] =
    {
        pEnt->WorldSpaceCenter(),
        pEnt->EyePosition()
    };

    for ( auto& pos : checkSpots )
    {
        if ( CanSee( pos ) )
        {
            return true;
        }
    }

    return false;
}

void NPCR::CBaseSenses::OnHeardSound( CSound* pSound )
{
    auto* pSrc = pSound->m_hOwner.Get();

    auto* pKnown = FindKnownOf( pSrc );
    if ( pKnown )
    {
        pKnown->UpdateLastSensed();
    }
}

bool NPCR::CBaseSenses::IsDebugging() const
{
    return npcr_debug_senses.GetInt() == GetOuter()->entindex() || npcr_debug_senses.GetInt() == 1;
}
