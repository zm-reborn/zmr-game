#pragma once

#include "soundent.h"

#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_zombie_chase.h"


class CombatSchedule : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    ChaseSched* m_pChaseSched;

    Vector m_vecFaceTowards;
    CountdownTimer m_FaceTimer;
    CountdownTimer m_NextEnemyScan;

    // For checking a potential threat
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_NextMove;


    CHandle<CBaseEntity> m_hLastEnemy;
    float m_flLastAlertSoundTime;
public:
    CombatSchedule()
    {
        m_vecFaceTowards = vec3_origin;

        m_pChaseSched = new ChaseSched;


        m_hLastEnemy.Set( nullptr );
        m_flLastAlertSoundTime = 0.0f;
    }

    ~CombatSchedule()
    {
        delete m_pChaseSched;
    }

    virtual const char* GetName() const OVERRIDE { return "ZombieCombatMonitor"; }

    virtual void OnStart() OVERRIDE
    {
    }

    virtual void OnContinue() OVERRIDE
    {
        m_FaceTimer.Invalidate();
        m_NextEnemyScan.Invalidate();

        m_Path.Invalidate();
        m_NextMove.Invalidate();
    }

    virtual void OnUpdate() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

        // See if outer wants to override us.
        // Banshee ceiling ambush
        NPCR::CSchedule<CZMBaseZombie>* pNewSched = pOuter->OverrideCombatSchedule();
        if ( pNewSched )
        {
            Intercept( pNewSched, "Overriding combat schedule..." );
            if ( IsIntercepted() )
                return;
        }


        
        bool bCanMove = pOuter->IsBusy() != NPCR::RES_YES;

        if ( bCanMove )
        {
            // Face towards possible danger.
            if ( !pOuter->GetMotor()->IsMoving() && m_FaceTimer.HasStarted() && !m_FaceTimer.IsElapsed() )
            {
                pOuter->GetMotor()->FaceTowards( m_vecFaceTowards );

                if ( pOuter->GetMotor()->IsFacing( m_vecFaceTowards, 10.0f ) )
                {
                    m_FaceTimer.Invalidate();
                }
            }


            // If we have a path, use that instead.
            if ( m_Path.IsValid() )
            {
                m_Path.Update( pOuter );
            }
        }
        else
        {
            if ( m_Path.IsValid() )
            {
                m_Path.Invalidate();
            }
        }


        // Check for any enemies we might see.
        if ( !m_NextEnemyScan.HasStarted() || m_NextEnemyScan.IsElapsed() )
        {
            CBaseEntity* pClosest = pOuter->GetSenses()->GetClosestEntity();
            CBaseEntity* pOldEnemy = pOuter->GetEnemy();

            // Make sure our current enemy is valid.
            if ( pOldEnemy && !pOuter->IsEnemy( pOldEnemy ) )
            {
                pOuter->LostEnemy();
                pOldEnemy = nullptr;
            }

            CBaseEntity* pEnemy = pClosest && pOuter->IsEnemy( pClosest ) ? pClosest : pOldEnemy;

            if ( bCanMove && pEnemy && pOuter->ShouldChase( pEnemy ) != NPCR::RES_NO )
            {
                pOuter->AcquireEnemy( pEnemy );

                CBaseEntity* pLastEnemy = m_hLastEnemy.Get();

                if ( !pOldEnemy )
                {
                    // Don't spam alert sound if we recently played it.
                    float quietTime = pLastEnemy == pEnemy ? 12.0f : 5.0f;

                    if ( (m_flLastAlertSoundTime+quietTime) < gpGlobals->curtime )
                    {
                        pOuter->AlertSound();

                        m_flLastAlertSoundTime = gpGlobals->curtime;
                    }

                    pOuter->RemoveSpawnFlags( SF_NPC_GAG );
                }


                m_hLastEnemy.Set( pEnemy );
                

                Intercept( m_pChaseSched, "We see a potential enemy!" );
                return;
            }

            m_NextEnemyScan.Start( 0.2f );
        }
    }

    virtual void OnSightGained( CBaseEntity* pEnt ) OVERRIDE
    {
        if ( GetOuter()->IsEnemy( pEnt ) )
        {
            m_NextEnemyScan.Invalidate();
            return;
        }
    }

    virtual void OnDamaged( const CTakeDamageInfo& info ) OVERRIDE
    {
        CBaseEntity* pAttacker = info.GetAttacker();
        if ( pAttacker && GetOuter()->IsEnemy( pAttacker ) )
        {
            m_vecFaceTowards = pAttacker->GetAbsOrigin();
            m_FaceTimer.Start( 2.0f );
        }
    }

    virtual void OnHeardSound( CSound* pSound ) OVERRIDE
    {
        auto* pOuter = GetOuter();

        // Try to look for any enemies we can hear.
        if ( !pOuter->GetEnemy() && !m_FaceTimer.HasStarted() && pSound->IsSoundType( SOUND_COMBAT | SOUND_PLAYER ) )
        {
            const Vector sndOrigin = pSound->GetSoundReactOrigin();
            
            
            
            bool bFaceSound =
                // We should be able to see that location, just turn around.
                pOuter->GetSenses()->CanSee( sndOrigin )
                // If we can't see it, and we're in offensive mode, try moving there.
            ||  (pOuter->GetZombieMode() != ZOMBIEMODE_OFFENSIVE || !GotoThreatPosition( sndOrigin ));

            if ( bFaceSound )
            {
                m_vecFaceTowards = sndOrigin;
                m_FaceTimer.Start( 2.0f );

                if ( IsDebugging() )
                    Msg( "Facing zombie %i towards sound.\n", pOuter->entindex() );
            }
        }
    }

    virtual void OnCommanded( ZombieCommandType_t com ) OVERRIDE
    {
        if ( m_Path.IsValid() )
            m_Path.Invalidate();

        m_FaceTimer.Invalidate();
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
    {
        OnCommanded( com );
    }

    bool GotoThreatPosition( const Vector& vecEnd )
    {
        if ( !m_NextMove.IsElapsed() )
            return false;


        CZMBaseZombie* pOuter = GetOuter();


        // Don't bother investigating if we're moving right now.
        if ( pOuter->IsMoving() )
            return false;


        const Vector vecStart = pOuter->GetAbsOrigin();

        CNavArea* start = TheNavMesh->GetNearestNavArea( vecStart, true, 10000.0f, true );
        CNavArea* goal = TheNavMesh->GetNearestNavArea( vecEnd, true, 10000.0f, true );


        m_PathCost = *pOuter->GetPathCost();
        m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );

        m_PathCost.SetStartPos( vecStart, start );
        m_PathCost.SetGoalPos( vecEnd, goal );


        m_Path.Compute( vecStart, vecEnd, start, goal, m_PathCost );
        if ( !m_Path.IsValid() )
        {
            if ( IsDebugging() )
                Msg( "Couldn't compute combat check path!\n" );
            return false;
        }


        pOuter->SetCurrentPath( &m_Path );

        // We don't need to get too close
        m_Path.SetGoalTolerance( 72.0f );



        if ( IsDebugging() )
            Msg( "Moving zombie %i towards sound.\n", pOuter->entindex() );


        m_FaceTimer.Invalidate();

        // Don't recompute this path again for a while.
        m_NextMove.Start( 2.0f );


        return true;
    }
};
