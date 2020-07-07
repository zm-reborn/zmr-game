#pragma once

#include "npcs/zmr_playerbot.h"

#include "zmr_survivor_combat.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

class BotMainSchedule : public NPCR::CSchedule<CZMPlayerBot>
{
private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    CountdownTimer m_ExpireTimer;
public:
    BotMainSchedule()
    {
    }

    ~BotMainSchedule()
    {
    }

    virtual const char* GetName() const OVERRIDE { return "PlayerMain"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return new SurvivorCombatSchedule; }

    virtual void OnStart() OVERRIDE
    {
        //m_ExpireTimer.Start( 1.0f );
    }

    virtual void OnUpdate() OVERRIDE
    {
        /*if ( m_Path.IsValid() && !m_ExpireTimer.IsElapsed() )
        {
            m_Path.Update( GetOuter() );
        }
        else
        {
            ComputePath();
            m_ExpireTimer.Start( 10.0f );
        }*/
    }
    
    void ComputePath()
    {
        CZMPlayerBot* pOuter = GetOuter();
        Vector vecMyPos = pOuter->GetAbsOrigin();
        CNavArea* pStart = pOuter->GetLastKnownArea();
        CNavArea* pGoal = nullptr;
        int n = random->RandomInt( 0, TheNavMesh->GetNavAreaCount() - 1 );

        // GCC doesn't like lambdas.
        /*
        TheNavMesh->ForAllAreas( [ &n, &pGoal ]( CNavArea* area )
        {
            if ( --n <= 0 )
            {
                pGoal = area;
                return false;
            }

            return true;
        } );
        */
        // Put me somewhere nice. I'll be useful in the future.
        class CAreaPick
        {
        public:
            CAreaPick( int n )
            {
                m_nCount = n;
                m_pArea = nullptr;
            }

            bool operator()( CNavArea* pArea )
            {
                if ( --m_nCount < 0 )
                {
                    m_pArea = pArea;
                    return false;
                }

                return true;
            }

            CNavArea* GetArea() const { return m_pArea; }

        private:
            CNavArea* m_pArea;
            int m_nCount;
        };

        CAreaPick pick( n );
        TheNavMesh->ForAllAreas( pick );

        pGoal = pick.GetArea();

        if ( !pGoal )
            return;

        Vector vecPos = pGoal->GetCenter();

        m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );
        m_PathCost.SetStartPos( vecMyPos, pStart );
        //m_PathCost.SetGoalPos( vecPos, pGoal );

        m_Path.Compute( vecMyPos, vecPos, pStart, pGoal, m_PathCost );

        pOuter->SetCurrentPath( m_Path.IsValid() ? &m_Path : nullptr );
    }
};
