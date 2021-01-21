#pragma once

#include "soundent.h"

#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_zombie_chase.h"


ConVar zm_sv_zombie_threat_investigation_dist( "zm_sv_zombie_threat_investigation_maxdist", "700" );


class CombatSchedule : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    ChaseSched* m_pChaseSched;

    Vector m_vecFaceTowards;
    CountdownTimer m_FaceTimer;
    CountdownTimer m_NextThreat;


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
        m_NextThreat.Invalidate();

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
            // Make sure our current enemy is valid.
            CBaseEntity* pOldEnemy = pOuter->GetEnemy();
            if ( pOldEnemy && !pOuter->IsEnemy( pOldEnemy ) )
            {
                pOuter->LostEnemy();
                pOldEnemy = nullptr;
            }

            // Find an enemy to attack!
            CBaseEntity* pEnemy = EvaluateEnemies();

            if ( pEnemy )
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

    virtual void OnAcquiredEnemy( CBaseEntity* pEnt ) OVERRIDE
    {
        OnSightGained( pEnt );
    }

    virtual void OnSightGained( CBaseEntity* pEnt ) OVERRIDE
    {
        //
        // Don't sleep any longer if we see a new enemy!
        //
        if ( GetOuter()->IsEnemy( pEnt ) )
        {
            m_NextEnemyScan.Invalidate();
            return;
        }
    }

    virtual void OnDamaged( const CTakeDamageInfo& info ) OVERRIDE
    {
        // Face towards our attacker.
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
        if ( !pOuter->GetEnemy() && ShouldCareAboutThreat() && pSound->IsSoundType( SOUND_COMBAT | SOUND_PLAYER ) )
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
                m_NextThreat.Start( 5.0f );

                if ( IsDebugging() )
                    Msg( "Facing zombie %i towards sound.\n", pOuter->entindex() );
            }
        }
    }

    virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) OVERRIDE
    {
        OnContinue();

        // We were command to do something else,
        // don't care about threats for a while.
        m_NextThreat.Start( 3.0f );
        // Definitely don't move for a while!
        m_NextMove.Start( 6.0f );
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
    {
        OnContinue();
    }

    bool ShouldCareAboutThreat() const
    {
        // We're currently trying to face towards something, not now!
        if ( m_FaceTimer.HasStarted() && !m_FaceTimer.IsElapsed() )
            return false;

        return !m_NextThreat.HasStarted() || m_NextThreat.IsElapsed();
    }

    //
    // There's something interesting near-by, go investigate.
    //
    bool GotoThreatPosition( const Vector& vecEnd )
    {
        if ( !m_NextMove.IsElapsed() )
            return false;


        CZMBaseZombie* pOuter = GetOuter();


        // Don't bother investigating if we're moving right now.
        if ( pOuter->IsMoving() )
            return false;


        const Vector vecStart = pOuter->GetAbsOrigin();


        float flInvestigationDist = zm_sv_zombie_threat_investigation_dist.GetFloat();

        if ( vecStart.DistTo( vecEnd ) > flInvestigationDist )
        {
            if ( IsDebugging() )
                Msg( "Investigation sound origin for %i is too far away!\n", pOuter->entindex() );

            return false;
        }



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


        float flPathLength = m_Path.ComputeLength();

        if ( flPathLength > flInvestigationDist )
        {
            if ( IsDebugging() )
                Msg( "Investigation path for %i is too long! (%.1f)\n", pOuter->entindex(), flPathLength );
            
            m_Path.Invalidate();

            // Don't attempt this path again for a while, it sounds expensive.
            m_NextMove.Start( 2.0f );

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
        m_NextThreat.Start( 8.0f );


        return true;
    }

    //
    // Find an enemy to attack!
    //
    CBaseEntity* EvaluateEnemies()
    {
        auto* pOuter = GetOuter();

        //
        // Iterate through our enemies.
        // Pick the closest one we can find.
        // And also the newest acquired one in case
        // of the ZM command style.
        //
        CBaseEntity* pClosest = nullptr;
        float flClosestDistSqr = FLT_MAX;

        CBaseEntity* pNewestAcquiredEnemy = nullptr;
        float flAcquiredDistSqr = FLT_MAX;
        float flAcquiredTime = 0.0f;

        const Vector vecMyPos = pOuter->GetAbsOrigin();

        const bool bMoving = pOuter->GetMotor()->IsMoving();

        pOuter->GetSenses()->IterateKnown( [ & ]( const NPCR::KnownEntity* pKnown )
        {
            auto* pEnt = pKnown->GetEntity();
            if ( !pEnt ) return false;

            if ( !pOuter->IsEnemy( pEnt ) || pOuter->ShouldChase( pEnt ) == NPCR::RES_NO )
            {
                return false;
            }


            float distSqr = pEnt->GetAbsOrigin().DistToSqr( vecMyPos );

            // Pick closest.
            // The closest one must be seen
            // or we heard them recently
            // or they are within attack range.
            float flLastSensed = gpGlobals->curtime - pKnown->LastSensedTime();
            if ( pKnown->CanSee() || flLastSensed < 0.5f || pOuter->HasConditionsForClawAttack( pEnt ) )
            {
                if ( !pClosest || distSqr < flClosestDistSqr )
                {
                    pClosest = pEnt;
                    flClosestDistSqr = distSqr;
                }
            }


            // Pick newest acquired.
            if ( pKnown->AcquiredTime() > flAcquiredTime )
            {
                if ( !pNewestAcquiredEnemy || distSqr < flAcquiredDistSqr )
                {
                    pNewestAcquiredEnemy = pEnt;
                    flAcquiredDistSqr = distSqr;
                    flAcquiredTime = pKnown->AcquiredTime();
                }
            }



            return false;
        } );


        // The zombie may only have a free will if:
        // it's allowed to have one or 
        // it's not moving or it can attack somebody!
        bool bHasFreeWill = pOuter->GetMyCommandStyle() != ZCOMMANDSTYLE_OLD
                        ||  !bMoving
                        ||  (pClosest && pOuter->HasConditionsForClawAttack( pClosest ));


        if ( bHasFreeWill )
        {
            // Go after the closest one we got!
            if ( pClosest )
            {
                return pClosest;
            }
        }

        // Quit here if the newest one was found
        // before us being commanded.
        if ( flAcquiredTime < pOuter->GetLastTimeCommanded() )
            return nullptr;

        // If we don't have a free will, only go after enemies
        // we've newly acquired.
        if ( pNewestAcquiredEnemy )
        {
            return pNewestAcquiredEnemy;
        }

        return nullptr;
    }
};
