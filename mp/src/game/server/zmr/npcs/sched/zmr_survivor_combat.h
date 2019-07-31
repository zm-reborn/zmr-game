#pragma once

#include "soundent.h"

#include "zmr/npcs/zmr_playerbot.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_survivor_attack.h"
#include "zmr_survivor_attack_closerange.h"
#include "zmr_survivor_attack_longrange.h"
#include "zmr_survivor_follow.h"


extern ConVar zm_sv_debug_bot_lookat;

class SurvivorCombatSchedule : public NPCR::CSchedule<CZMPlayerBot>
{
private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_NextEnemyScan;
    CountdownTimer m_NextRangeCheck;

    CountdownTimer m_NextHeardLook;
    CountdownTimer m_NextLookAround;
    CNavArea* m_pLastLookArea;
    Vector m_vecLookAt;

    bool m_bMovingOutOfRange;
    CHandle<CBaseEntity> m_hLastCombatTarget;

    SurvivorAttackCloseRangeSched* m_pAttackCloseRangeSched;
    SurvivorAttackLongRangeSched* m_pAttackLongRangeSched;
public:
    SurvivorCombatSchedule()
    {
        m_pAttackCloseRangeSched = new SurvivorAttackCloseRangeSched;
        m_pAttackLongRangeSched = new SurvivorAttackLongRangeSched;

        m_vecLookAt = vec3_origin;
        m_pLastLookArea = nullptr;
    }

    ~SurvivorCombatSchedule()
    {
        delete m_pAttackCloseRangeSched;
        delete m_pAttackLongRangeSched;
    }

    virtual const char* GetName() const OVERRIDE { return "SurvivorCombatMonitor"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return new SurvivorFollowSchedule; }

    virtual void OnStart() OVERRIDE
    {
        m_NextEnemyScan.Invalidate();
        m_NextRangeCheck.Invalidate();
        m_NextHeardLook.Invalidate();
        m_NextLookAround.Invalidate();
        m_hLastCombatTarget.Set( nullptr );
    }

    virtual void OnContinue() OVERRIDE
    {
        m_Path.Invalidate();
        m_NextLookAround.Start( 4.0f );
    }

    virtual void OnIntercept() OVERRIDE
    {
        m_Path.Invalidate();
    }

    virtual void OnUpdate() OVERRIDE
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


        bool bBusy = pOuter->IsBusy() == NPCR::RES_YES;

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

    virtual void OnSpawn() OVERRIDE
    {
        m_Path.Invalidate();
    }

    virtual void OnHeardSound( CSound* pSound ) OVERRIDE
    {
        if ( !m_bMovingOutOfRange && (!m_NextHeardLook.HasStarted() || m_NextHeardLook.IsElapsed()) )
        {
            m_NextHeardLook.Start( 5.0f );
            m_NextLookAround.Start( 3.0f );
            m_vecLookAt = pSound->GetSoundOrigin();
        }
    }

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
    }

    bool ShouldMoveBack( CBaseEntity* pEnemy ) const
    {
        float flDistSqr = pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() );

        // ZMRTODO: Take into account velocity.
        if ( flDistSqr > (128.0f*128.0f) )
            return false;


        return true;
    }

    void MoveBackFromThreat( CBaseEntity* pEnemy )
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
};
