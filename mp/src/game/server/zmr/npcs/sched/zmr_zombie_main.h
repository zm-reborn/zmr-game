#pragma once

#include "zmr/npcs/zmr_zombiebase.h"

#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

#include "zmr_zombie_combat.h"


extern ConVar zm_sv_defense_chase_dist;
extern ConVar zm_sv_defense_goal_tolerance;



class MoveSchedule : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    Vector m_vecCurrentGoal;
    bool m_bHasGoal;
    CountdownTimer m_BlockerTimer;
    GotoSwatObjSched* m_pGotoSwatSched;

public:
    MoveSchedule()
    {
        m_bHasGoal = false;
        m_vecCurrentGoal = vec3_origin;
        m_pGotoSwatSched = new GotoSwatObjSched;
    }

    ~MoveSchedule()
    {
        delete m_pGotoSwatSched;
    }

    virtual const char* GetName() const OVERRIDE { return "ZombieControlMonitor"; }

    virtual NPCR::CSchedule<CZMBaseZombie>* CreateFriendSchedule() OVERRIDE { return new CombatSchedule; }

    virtual void OnStart() OVERRIDE
    {
        m_PathCost = *GetOuter()->GetPathCost();
    }

    virtual void OnUpdate() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

            
        if ( m_Path.IsValid() )
        {
            // See if we have anything blocking us.
            if ( m_BlockerTimer.IsElapsed() && pOuter->GetBlockerFinder()->GetTimesBlocked() > 1 && pOuter->GetBlockerFinder()->GetBlocker() )
            {
                CBaseEntity* pBlocker = pOuter->GetBlockerFinder()->GetBlocker();

                bool bSwat = pOuter->CanSwatPhysicsObjects() && CZMBaseZombie::CanSwatObject( pBlocker );
                bool bBreak = pOuter->CanBreakObject( pBlocker, true );
                if ( bSwat || bBreak )
                {
                    pOuter->GetBlockerFinder()->ClearBlocker();
                    pOuter->GetBlockerFinder()->ClearTimesBlocked();

                    pOuter->SetSwatObject( pBlocker );


                    Intercept( m_pGotoSwatSched, "Clear the blocking entity!" );
                    return;
                }

                m_BlockerTimer.Start( 0.5f );
            }


            if ( pOuter->CanMove() )
            {
                m_Path.Update( pOuter );
            }
        }
        else if ( pOuter->GetZombieMode() == ZOMBIEMODE_DEFEND )
        {
            const Vector defpos = GetGoalPos();

            float goalsqr = zm_sv_defense_goal_tolerance.GetFloat();
            goalsqr *= goalsqr;
            if ( pOuter->GetPosition().DistToSqr( defpos ) > goalsqr )
            {
                CBaseEntity* pEnemy = pOuter->GetEnemy();

                float chasesqr = zm_sv_defense_chase_dist.GetFloat();
                chasesqr *= chasesqr;
                // Command the zombie back to start.
                if ( pEnemy && !ShouldDefendFrom( pEnemy ) )
                {
                    Command( defpos );
                    return;
                }
            }
        }


        CZMCommandBase* pQueued = pOuter->GetCommandQueue()->NextCommand();
        if ( pQueued )
        {
            if ( IsBusy() != NPCR::RES_YES )
            {
                bool bRes = true;

                switch ( pQueued->GetCommandType() )
                {
                case COMMAND_MOVE :
                    Command( pQueued->GetVectorTarget() );
                    break;
                case COMMAND_SWAT :
                    CommandSwat( pQueued->GetObjectTarget(), static_cast<CZMCommandSwat*>( pQueued )->BreakObject() );
                    break;
                default :
                    bRes = false;
                    break;
                }

                if ( bRes )
                    pOuter->GetCommandQueue()->RemoveCommand( pQueued );
            }
        }
    }

    virtual void OnChase( CBaseEntity* pEnt ) OVERRIDE
    {
        m_Path.Invalidate();
    }

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

        if ( (gpGlobals->curtime - pOuter->GetLastTimeCommanded()) < 1.0f )
            return NPCR::RES_NO;

        if ( pOuter->GetZombieMode() == ZOMBIEMODE_AMBUSH )
            return NPCR::RES_NO;

        if ( pOuter->GetZombieMode() == ZOMBIEMODE_DEFEND && !ShouldDefendFrom( pEnemy ) )
        {
            return NPCR::RES_NO;
        }

        return NPCR::RES_NONE;
    }

    bool Command( const Vector& vecPos )
    {
        UpdateGoal( vecPos );


        CZMBaseZombie* pOuter = GetOuter();
        //pOuter->SetEnemy( nullptr );
        pOuter->UpdateLastTimeCommanded();


        Vector vecMyPos = pOuter->GetAbsOrigin();

        CNavArea* pStart = pOuter->GetLastKnownArea();
        if ( !pStart ) // We might not have a position yet if we just spawned.
        {
            pStart = TheNavMesh->GetNearestNavArea( pOuter->GetPosition(), true, 128.0f, true );
        }

        CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecPos, true, 128.0f, true );
        m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );
        m_PathCost.SetStartPos( vecMyPos, pStart );
        m_PathCost.SetGoalPos( vecPos, pGoal );


        // Set the goal tolerance based on how far the zombie is.
        const float flToleranceMinDist = 32.0f;
        const float flToleranceMaxDist = 192.0f;
        const float flMinTolerance = 6.0f;
        const float flMaxTolerance = 32.0f;

        float tolerance = SimpleSplineRemapValClamped(
            (vecPos - vecMyPos).Length(),
            flToleranceMinDist, flToleranceMaxDist,
            flMinTolerance,
            flMaxTolerance );
            
        m_Path.SetGoalTolerance( tolerance );


        m_Path.Compute( vecMyPos, vecPos, pStart, pGoal, m_PathCost );

        if ( m_Path.IsValid() )
            pOuter->SetCurrentPath( &m_Path );

        pOuter->OnCommanded( COMMAND_MOVE );

        return true;
    }

    bool CommandSwat( CBaseEntity* pEnt, bool bBreak = true )
    {
        if ( !pEnt )
            return false;


        CZMBaseZombie* pOuter = GetOuter();

            
        pOuter->SetSwatObject( pEnt );



        pOuter->OnCommanded( COMMAND_SWAT );


        m_pGotoSwatSched->SetBreakObject( bBreak );
        Intercept( m_pGotoSwatSched, "Commanded to swat this object!" );
        if ( IsIntercepted() )
        {
            // We have no use for the path anymore.
            m_Path.Invalidate();
        }

        return true;
    }

    bool ShouldDefendFrom( CBaseEntity* pEnemy ) const
    {
        float chasesqr = zm_sv_defense_chase_dist.GetFloat();
        chasesqr *= chasesqr;
        return pEnemy && pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() ) < chasesqr;
    }

    const Vector& GetGoalPos()
    {
        if ( !m_bHasGoal )
        {
            UpdateGoal( GetNPC()->GetPosition() );
        }

        return m_vecCurrentGoal;
    }

    void UpdateGoal( const Vector& vecPos )
    {
        m_bHasGoal = true;
        m_vecCurrentGoal = vecPos;
    }
};
