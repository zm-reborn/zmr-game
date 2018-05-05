#pragma once

#include "zmr/npcs/zmr_playerbot.h"
//#include "zmr/npcs/zmr_zombiebase.h"

#include "zmr_survivor_attack.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

class SurvivorAttackCloseRangeSched : public NPCR::CSchedule<CZMPlayerBot>, public CZMSurvivorAttackInt
{
private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    bool m_bMovingToRange;
    bool m_bMovingOutOfRange;
    CountdownTimer m_NextRangeCheck;
public:
    SurvivorAttackCloseRangeSched()
    {
    }

    ~SurvivorAttackCloseRangeSched()
    {
    }

    virtual const char* GetName() const OVERRIDE { return "SurvivorCombatCloseRange"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return nullptr; }

    virtual void OnStart() OVERRIDE
    {
        m_bMovingToRange = false;
        m_Path.Invalidate();
        m_NextRangeCheck.Invalidate();


        CZMPlayerBot* pOuter = GetOuter();

        if ( pOuter->EquipWeaponOfType( BOTWEPRANGE_CLOSERANGE ) && pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
        {
            return;
        }

        if ( pOuter->EquipWeaponOfType( BOTWEPRANGE_LONGRANGE ) && pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
        {
            return;
        }

        //pOuter->EquipWeaponOfType( BOTWEPRANGE_MELEE );
        End( "Couldn't find any weapons with ammo!" );
    }

    virtual void OnContinue() OVERRIDE
    {
        End( "Done." );
    }

    virtual void OnUpdate() OVERRIDE
    {
        CZMPlayerBot* pOuter = GetOuter();

        CBaseEntity* pEnemy = GetAttackTarget();
        if ( !pEnemy || !pEnemy->IsAlive() )
        {
            End( "No enemy left!" );
            return;
        }


        if ( !pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
        {
            End( "No ammo left!" );
            return;
        }


        if ( !m_NextRangeCheck.HasStarted() || m_NextRangeCheck.IsElapsed() )
        {
            m_NextRangeCheck.Start( 0.1f );


            if ( !m_bMovingOutOfRange && ShouldMoveBack( pEnemy ) )
            {
                MoveBackFromThreat( pEnemy );
            }
            else if ( !m_bMovingToRange && !IsInRangeToAttack( pEnemy ) )
            {
                MoveToShootingRange( pEnemy );
            }
        }


        if ( m_Path.IsValid() )
        {
            m_Path.Update( pOuter );
        }
        else
        {
            m_bMovingOutOfRange = false;
            m_bMovingToRange = false;
        }


        Vector vecAimTarget;


        vecAimTarget = pEnemy->WorldSpaceCenter();

        pOuter->GetMotor()->FaceTowards( vecAimTarget );

        if ( IsInRangeToAttack( pEnemy ) && pOuter->GetMotor()->IsFacing( vecAimTarget, 8.0f ) )
        {
            pOuter->PressFire1( 0.1f );
        }
    }

    bool IsInRangeToAttack( CBaseEntity* pEnemy ) const
    {
        return pEnemy->GetAbsOrigin().DistTo( GetOuter()->GetPosition() ) < 220.0f;
    }

    bool ShouldMoveBack( CBaseEntity* pEnemy ) const
    {
        float flDistSqr = pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() );

        if ( flDistSqr > (128.0f*128.0f) )
            return false;


        return true;
    }

    void MoveToShootingRange( CBaseEntity* pEnemy )
    {
        CZMPlayerBot* pOuter = GetOuter();

        const Vector vecEnemyPos = pEnemy->GetAbsOrigin();
        const Vector vecMyPos = pOuter->GetPosition();

        Vector dir = (vecMyPos - vecEnemyPos).Normalized();

        Vector vecTarget = vecEnemyPos + dir * 128.0f;

        CNavArea* pStart = pOuter->GetLastKnownArea();
        CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecTarget, true, 128.0f, false );

        if ( pGoal )
        {
            pGoal->GetClosestPointOnArea( vecTarget, &vecTarget );
        }

        m_PathCost.SetStartPos( vecMyPos, pStart );
        m_PathCost.SetGoalPos( vecTarget, pGoal );

        m_Path.Compute( vecMyPos, vecTarget, pStart, pGoal, m_PathCost );


        m_bMovingOutOfRange = false;
        m_bMovingToRange = m_Path.IsValid();
    }

    void MoveBackFromThreat( CBaseEntity* pEnemy )
    {
        CZMPlayerBot* pOuter = GetOuter();

        const Vector vecEnemyPos = pEnemy->GetAbsOrigin();
        const Vector vecMyPos = pOuter->GetPosition();

        Vector dir = (vecMyPos - vecEnemyPos).Normalized();

        Vector vecTarget = vecMyPos + dir * 128.0f;

        CNavArea* pStart = pOuter->GetLastKnownArea();
        CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecTarget, true, 128.0f, false );

        if ( pGoal )
        {
            pGoal->GetClosestPointOnArea( vecTarget, &vecTarget );
        }

        m_PathCost.SetStartPos( vecMyPos, pStart );
        m_PathCost.SetGoalPos( vecTarget, pGoal );

        m_Path.Compute( vecMyPos, vecTarget, pStart, pGoal, m_PathCost );


        m_bMovingOutOfRange = m_Path.IsValid();
        m_bMovingToRange = false;
    }
};
