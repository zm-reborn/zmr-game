#include "cbase.h"

#include "zmr_survivor_follow.h"
#include "zmr_survivor_combat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar zm_sv_debug_bot_lookat;


CSurvivorCombatSchedule::CSurvivorCombatSchedule()
{
    m_pAttackCloseRangeSched = new CSurvivorAttackCloseRangeSchedule;
    m_pAttackLongRangeSched = new CSurvivorAttackLongRangeSchedule;

    m_vecLookAt = vec3_origin;
    m_pLastLookArea = nullptr;
}

CSurvivorCombatSchedule::~CSurvivorCombatSchedule()
{
    delete m_pAttackCloseRangeSched;
    delete m_pAttackLongRangeSched;
}


NPCR::CSchedule<CZMPlayerBot>* CSurvivorCombatSchedule::CreateFriendSchedule()
{
    return new CSurvivorFollowSchedule;
}

void CSurvivorCombatSchedule::OnStart()
{
    m_NextEnemyScan.Invalidate();
    m_NextRangeCheck.Invalidate();
    m_NextHeardLook.Invalidate();
    m_NextLookAround.Invalidate();
    m_hLastCombatTarget.Set( nullptr );
}

void CSurvivorCombatSchedule::OnContinue()
{
    m_Path.Invalidate();
    m_NextLookAround.Start( 4.0f );
}

void CSurvivorCombatSchedule::OnIntercept()
{
    m_Path.Invalidate();
}

void CSurvivorCombatSchedule::OnUpdate()
{
    CZMPlayerBot* pOuter = GetOuter();

    if ( !pOuter->IsHuman() )
    {
        return;
    }



    if ( !m_NextEnemyScan.HasStarted() || m_NextEnemyScan.IsElapsed() )
    {
        m_NextEnemyScan.Start( 0.2f );


        CBaseEntity* pClosestEnemy = pOuter->GetSenses()->GetClosestEntity();

        //CBaseEntity* pCurEnemy = m_hLastCombatTarget.Get();

        if ( pClosestEnemy && pOuter->ShouldChase( pClosestEnemy ) != NPCR::RES_NO )
        {
            m_hLastCombatTarget.Set( pClosestEnemy );

            if ( pOuter->HasAnyEffectiveRangeWeapons() )
            {
                if ( pClosestEnemy->GetAbsOrigin().DistTo( pOuter->GetPosition() ) > 400.0f )
                {
                    m_pAttackLongRangeSched->SetAttackTarget( pClosestEnemy );
                    Intercept( m_pAttackLongRangeSched, "Trying to attack long range!" );
                }

                if ( !IsIntercepted() )
                {
                    m_pAttackCloseRangeSched->SetAttackTarget( pClosestEnemy );
                    m_pAttackCloseRangeSched->SetMeleeing( false );
                    Intercept( m_pAttackCloseRangeSched, "Trying to attack close range!" );
                }
            }
            else if ( pClosestEnemy->GetAbsOrigin().DistTo( pOuter->GetPosition() ) < 160.0f )
            {
                m_pAttackCloseRangeSched->SetAttackTarget( pClosestEnemy );
                m_pAttackCloseRangeSched->SetMeleeing( true );
                Intercept( m_pAttackCloseRangeSched, "Trying to attack close range with melee!" );
            }

            if ( IsIntercepted() )
                return;
        }
    }


    SetSelfCall( true ); // Ignore our own IsBusy listener.
    bool bBusy = pOuter->IsBusy() == NPCR::RES_YES;
    SetSelfCall( false );

    if ( !bBusy && (!m_NextRangeCheck.HasStarted() || m_NextRangeCheck.IsElapsed()) )
    {
        m_NextRangeCheck.Start( 0.5f );

        CBaseEntity* pEnemy = m_hLastCombatTarget.Get();
        if ( pEnemy )
        {
            if ( ShouldMoveBack( pEnemy ) )
                MoveBackFromThreat( pEnemy );
        }
            
    }


    if ( m_Path.IsValid() && !bBusy )
    {
        m_Path.Update( pOuter );
    }
    else
    {
        m_Path.Invalidate();

        m_bMovingOutOfRange = false;

        if ( !m_NextLookAround.HasStarted() || m_NextLookAround.IsElapsed() )
        {
            m_NextLookAround.Start( random->RandomFloat( 4.0f, 10.0f ) );

            CNavArea* pMyArea = pOuter->GetLastKnownArea();

            if ( pMyArea )
            {
                CNavArea* pArea = nullptr;
                for ( int i = 0; i < NUM_DIRECTIONS; i++ )
                {
                    pArea = pMyArea->GetRandomAdjacentArea( (NavDirType)i );
                    if ( pArea && pArea != m_pLastLookArea )
                    {
                        m_pLastLookArea = pArea;
                        m_vecLookAt = pArea->GetRandomPoint() + Vector( 0.0f, 0.0f, 64.0f );
                        break;
                    }
                }
            }
        }
    }


    bool bIsFacing = pOuter->GetMotor()->IsFacing( m_vecLookAt );

    if ( zm_sv_debug_bot_lookat.GetBool() )
    {
        Vector box( 4.0f, 4.0f, 4.0f );
        NDebugOverlay::Box( m_vecLookAt, -box, box, (!bIsFacing) ? 255 : 0, bIsFacing ? 255 : 0, 0, 0, 0.1f );
    }

    if ( !bIsFacing )
    {
        pOuter->GetMotor()->FaceTowards( m_vecLookAt );
    }
}

void CSurvivorCombatSchedule::OnSpawn()
{
    m_Path.Invalidate();
}

void CSurvivorCombatSchedule::OnHeardSound( CSound* pSound )
{
    if ( !m_bMovingOutOfRange && (!m_NextHeardLook.HasStarted() || m_NextHeardLook.IsElapsed()) )
    {
        m_NextHeardLook.Start( 5.0f );
        m_NextLookAround.Start( 3.0f );


        auto* pOwner = pSound->m_hOwner.Get();

        if ( pOwner && pOwner->IsPlayer() )
        {
            // Face shooter's direction.
            Vector fwd;
            AngleVectors( pOwner->EyeAngles(), &fwd );
                
            m_vecLookAt = pOwner->EyePosition() + fwd * 1024.0f;
        }
        else
        {
            m_vecLookAt = pSound->GetSoundOrigin();
        }
            
    }
}

void CSurvivorCombatSchedule::OnForcedMove( CNavArea* pArea )
{
    m_Path.Invalidate();

    auto vecMyPos = GetOuter()->GetAbsOrigin();
    auto vecTarget = pArea->GetRandomPoint();

    auto* pStart = GetOuter()->GetLastKnownArea();

    m_PathCost.SetStartPos( vecMyPos, pStart );
    m_PathCost.SetGoalPos( vecTarget, pArea );

    m_Path.Compute( vecMyPos, vecTarget, pStart, pArea, m_PathCost );
}

NPCR::QueryResult_t CSurvivorCombatSchedule::IsBusy() const
{
    return (m_Path.IsValid() &&
            !IsSelfCall()) // Ignore if we're the one asking.
        ? NPCR::RES_YES : NPCR::RES_NONE;
}

bool CSurvivorCombatSchedule::ShouldMoveBack( CBaseEntity* pEnemy ) const
{
    float flDistSqr = pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() );

    // ZMRTODO: Take into account velocity.
    if ( flDistSqr > (128.0f*128.0f) )
        return false;


    return true;
}

void CSurvivorCombatSchedule::MoveBackFromThreat( CBaseEntity* pEnemy )
{
    CZMPlayerBot* pOuter = GetOuter();

    const Vector vecEnemyPos = pEnemy->GetAbsOrigin();
    const Vector vecMyPos = pOuter->GetPosition();

    Vector dir = (vecMyPos - vecEnemyPos);
    dir.z = 0.0f;
    dir.NormalizeInPlace();

    Vector vecTarget = vecMyPos + dir * 128.0f;
    Vector vecOriginalTarget = vecTarget;

    CNavArea* pStart = pOuter->GetLastKnownArea();
    CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecTarget, true, 200.0f, false );

    if ( pGoal )
    {
        pGoal->GetClosestPointOnArea( vecTarget, &vecTarget );

        // We don't want to go forward!
        if ( (vecTarget - vecMyPos).Normalized().Dot( dir ) < 0.0f )
        {
            vecTarget = vecOriginalTarget;
        }
    }

    m_PathCost.SetStartPos( vecMyPos, pStart );
    m_PathCost.SetGoalPos( vecTarget, pGoal );

    m_Path.Compute( vecMyPos, vecTarget, pStart, pGoal, m_PathCost );


    m_bMovingOutOfRange = m_Path.IsValid();

    m_vecLookAt = pEnemy->WorldSpaceCenter();
}
