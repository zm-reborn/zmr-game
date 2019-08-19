#pragma once

#include "soundent.h"

#include "zmr/npcs/zmr_playerbot.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_chase.h"


class SurvivorFollowSchedule : public NPCR::CSchedule<CZMPlayerBot>
{
private:
    NPCR::CChaseNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    CountdownTimer m_NextFollowTarget;

    CHandle<CBasePlayer> m_hFollowTarget;
public:
    SurvivorFollowSchedule()
    {
    }

    ~SurvivorFollowSchedule()
    {
    }

    virtual const char* GetName() const OVERRIDE { return "SurvivorFollowMonitor"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return nullptr; }

    virtual void OnStart() OVERRIDE
    {
        m_NextFollowTarget.Start( 2.0f );
    }

    virtual void OnContinue() OVERRIDE
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


        auto* pFollow = m_hFollowTarget.Get();


        if ( (pFollow && !IsValidFollowTarget( pFollow )) || m_NextFollowTarget.IsElapsed() )
        {
            NextFollow();

            pFollow = m_hFollowTarget.Get();
        }


        bool bBusy = pOuter->IsBusy() == NPCR::RES_YES;

        if ( m_Path.IsValid() && pFollow && !bBusy && ShouldMoveCloser( pFollow ) )
        {
            m_Path.Update( pOuter, pFollow, m_PathCost );
        }
    }

    virtual void OnSpawn() OVERRIDE
    {
        m_Path.Invalidate();

        m_NextFollowTarget.Start( 0.5f );
    }

    virtual void OnHeardSound( CSound* pSound ) OVERRIDE
    {
    }

    //virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    //{
    //    return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
    //}

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        auto* pFollow = m_hFollowTarget.Get();
        return pFollow && m_Path.IsValid() && ShouldMoveCloser( pFollow ) ? NPCR::RES_NO : NPCR::RES_NONE;
    }

    //virtual void OnMoveSuccess( NPCR::CBaseNavPath* pPath ) OVERRIDE
    //{
    //        
    //}

    bool IsValidFollowTarget( CBasePlayer* pPlayer ) const
    {
        if ( pPlayer->GetTeamNumber() != ZMTEAM_HUMAN || !pPlayer->IsAlive() )
        {
            return false;
        }

        if ( pPlayer->IsBot() )
        {
            auto* pBot = static_cast<CZMPlayerBot*>( pPlayer );
            if ( pBot->GetFollowTarget() == GetOuter() ) // Don't follow a bot that is following us, lul.
            {
                return false;
            }
        }

        return true;
    }

    void NextFollow()
    {
        //auto* pOuter = GetOuter();

        float flNextCheck = 1.0f;


        auto* pLastFollow = m_hFollowTarget.Get();
        bool bWasValid = pLastFollow ? IsValidFollowTarget( pLastFollow ) : false;

        auto* pFollow = FindSurvivorToFollow();

        if ( (pLastFollow != pFollow || !m_Path.IsValid()) && pFollow )
        {
            StartFollow( pFollow );

            //float dist = pOuter->GetAbsOrigin().DistTo( pFollow->GetAbsOrigin() );
            
            flNextCheck = 15.0f;
        }
        else if ( !bWasValid )
        {
            m_hFollowTarget.Set( nullptr );
        }

        m_NextFollowTarget.Start( flNextCheck );
    }

    void StartFollow( CBasePlayer* pFollow )
    {
        auto* pOuter = GetOuter();


        m_hFollowTarget.Set( pFollow );
        pOuter->SetFollowTarget( pFollow );

        m_Path.SetGoalTolerance( 0.0f );
        m_Path.Compute( pOuter, pFollow, m_PathCost );
    }

    bool ShouldMoveCloser( CBasePlayer* pFollow ) const
    {
        auto* pOuter = GetOuter();

        float flDistSqr = pOuter->GetAbsOrigin().DistToSqr( pFollow->GetAbsOrigin() );
        
        //if 
        {
            return ( flDistSqr > (128.0f * 128.0f));
        }
    }

    CBasePlayer* FindSurvivorToFollow( CBasePlayer* pIgnore = nullptr, bool bAllowBot = false ) const
    {
        auto* pOuter = GetOuter();

        Vector mypos = pOuter->GetAbsOrigin();

        CBasePlayer* pClosest = nullptr;
        float flClosestDist = FLT_MAX;

        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
            auto* pPlayer = static_cast<CBasePlayer*>( UTIL_EntityByIndex( i ) );

            if ( !pPlayer ) continue;

            if ( pPlayer == pOuter ) continue;

            if ( !bAllowBot && pPlayer->IsBot() ) continue;

            if ( !IsValidFollowTarget( pPlayer ) ) continue;


            float dist = pPlayer->GetAbsOrigin().DistToSqr( mypos );
            if ( dist < flClosestDist )
            {
                flClosestDist = dist;
                pClosest = pPlayer;
            }
        }

        if ( !bAllowBot )
        {
            // Try bot one.
            return FindSurvivorToFollow( pIgnore, true );
        }

        return pClosest;
    }
};
