#pragma once


#include "npcs/zmr_zombiebase.h"

#include "npcr_path_cost.h"
#include "npcr_path_chase.h"

#include "zmr_zombie_clawattack.h"
#include "zmr_zombie_swat_scan.h"
#include "zmr_zombie_swat_goto.h"


class ChaseSched : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    ClawAttackSched* m_pAttackSched;
    ScanSwatObjSched* m_pScanSwatSched;
    GotoSwatObjSched* m_pGotoSwatSched;
    CountdownTimer m_SwatScanTimer;
    CountdownTimer m_BlockerTimer;
    CountdownTimer m_EnemyScanTimer;
    CountdownTimer m_StuckTimer;

    NPCR::CChaseNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
public:
    ChaseSched()
    {
        m_pAttackSched = new ClawAttackSched;
        m_pScanSwatSched = new ScanSwatObjSched;
        m_pGotoSwatSched = new GotoSwatObjSched;
        // Make sure we're not going the opposite direction when trying to swat
        m_pGotoSwatSched->SetCheckDirection( true );
        // Scan for any enemies that are closer than the swatting object.
        m_pGotoSwatSched->SetCheckForEnemies( true );

        // We want to go exactly to the goal in case the enemy is not on nav mesh.
        m_Path.UseExactGoal( true );
    }
    ~ChaseSched()
    {
        delete m_pAttackSched;
        delete m_pScanSwatSched;
        delete m_pGotoSwatSched;
    }

    virtual const char* GetName() const OVERRIDE { return "ZombieChase"; }


    virtual void OnStart() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();
        CBaseEntity* pEnemy = pOuter->GetEnemy();
        if ( !pEnemy )
        {
            End( "No enemy to chase!" );
            return;
        }


        const Vector vecStart = pOuter->GetAbsOrigin();
        const Vector vecEnd = pEnemy->GetAbsOrigin();

        CNavArea* start = TheNavMesh->GetNearestNavArea( vecStart, true, 10000.0f, true );
        CNavArea* goal = TheNavMesh->GetNearestNavArea( vecEnd, true, 10000.0f, true );


        m_PathCost = *pOuter->GetPathCost();
        m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );

        m_PathCost.SetStartPos( vecStart, start );
        m_PathCost.SetGoalPos( vecEnd, goal );

        m_Path.Compute( vecStart, vecEnd, start, goal, m_PathCost );
        if ( !m_Path.IsValid() )
        {
            End( "Couldn't compute chase path!" );
            return;
        }


        pOuter->SetCurrentPath( &m_Path );


        // Zero out costs because chase update will recompute the path.
        m_PathCost.SetStartPos( vec3_origin, nullptr );
        m_PathCost.SetGoalPos( vec3_origin, nullptr );



        m_SwatScanTimer.Start( 3.0f );
        m_BlockerTimer.Start( 0.2f );
        m_StuckTimer.Invalidate();

        pOuter->OnChase( pEnemy );
    }

    virtual void OnContinue() OVERRIDE
    {
        End( "Done chasing!" );
    }

    virtual void OnEnd() OVERRIDE
    {
        GetOuter()->SetCurrentPath( nullptr );
    }

    virtual void OnUpdate() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

        CBaseEntity* pEnemy = pOuter->GetEnemy();
        if ( !pEnemy )
        {
            End( "No enemy to chase!" );
            return;
        }

        if ( !pOuter->IsEnemy( pEnemy ) )
        {
            pOuter->LostEnemy();

            End( "Enemy is no longer enemy! wut?" );
            return;
        }


        // Scan for a better target to chase.
        if ( !m_EnemyScanTimer.HasStarted() || m_EnemyScanTimer.IsElapsed() )
        {
            CBaseEntity* pEnt = pOuter->GetSenses()->GetClosestEntity();
            if ( pEnt && pEnemy != pEnt && pOuter->IsEnemy( pEnt ) )
            {
                const Vector vecMyPos = pOuter->WorldSpaceCenter();

                float flCurDist = vecMyPos.DistTo( pEnemy->WorldSpaceCenter() );
                float flNewDist = vecMyPos.DistTo( pEnt->WorldSpaceCenter() );

                // We have to be reasonably closer to the potential enemy.
                if ( flNewDist < (flCurDist*0.8f) )
                {
                    pOuter->AcquireEnemy( pEnt );
                }
            }

            m_EnemyScanTimer.Start( 0.5f );
        }


        if ( pOuter->CanMove() )
        {
            if ( pOuter->HasConditionsForClawAttack( pEnemy ) )
            {
                Intercept( m_pAttackSched, "Close enough to attack!" );
                return;
            }

            if ( pOuter->HasConditionsForRangeAttack( pEnemy ) )
            {
                auto* pSched = pOuter->GetRangeAttackSchedule();
                if ( pSched )
                {
                    Intercept( pSched, "Start range attack!" );
                    return;
                }
            }
        }


        // See if we have anything blocking us.
        if ( m_BlockerTimer.IsElapsed() && pOuter->GetBlockerFinder()->GetTimesBlocked() > 1 && pOuter->GetBlockerFinder()->GetBlocker() )
        {
            CBaseEntity* pBlocker = pOuter->GetBlockerFinder()->GetBlocker();

            // Only swat if we've been stuck for a while.
            bool bSwat =    pOuter->CanSwatPhysicsObjects()
                    &&      CZMBaseZombie::CanSwatObject( pBlocker )
                    &&      m_StuckTimer.HasStarted()
                    &&      !m_StuckTimer.IsElapsed();

            bool bBreak = pOuter->CanBreakObject( pBlocker, true );
            if ( bSwat || bBreak )
            {
                pOuter->GetBlockerFinder()->ClearBlocker();
                pOuter->GetBlockerFinder()->ClearTimesBlocked();

                pOuter->SetSwatObject( pBlocker );

                // Don't bother spending more than this going to the blocker
                m_pGotoSwatSched->SetStartExpireTime( 1.0f );
                m_pGotoSwatSched->SetBreakObject( true );

                Intercept( m_pGotoSwatSched, "Clear the blocking entity!" );
                return;
            }

            m_BlockerTimer.Start( 0.5f );
        }


        if ( pOuter->CanSwatPhysicsObjects() && m_SwatScanTimer.IsElapsed() )
        {
            m_SwatScanTimer.Start( 3.0f );

            m_pScanSwatSched->SetBreakObject( false );
            Intercept( m_pScanSwatSched, "Try scanning swatable objects when chasing." );
            if ( IsIntercepted() )
                return;
        }


        if ( pOuter->CanMove() )
        {
            m_Path.Update( pOuter, pEnemy->MyCombatCharacterPointer(), m_PathCost );
        }
    }

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    virtual void OnStuck() OVERRIDE
    {
        m_StuckTimer.Start( 1.0f );
    }
};
