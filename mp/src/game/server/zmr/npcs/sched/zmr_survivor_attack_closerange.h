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
    CountdownTimer m_NextMovingToRange;

    bool m_bAllowMelee;
public:
    SurvivorAttackCloseRangeSched()
    {
        m_bAllowMelee = false;
    }

    ~SurvivorAttackCloseRangeSched()
    {
    }

    void SetMeleeing( bool value ) { m_bAllowMelee = value; }
    bool IsMeleeing() const { return m_bAllowMelee; }

    virtual const char* GetName() const OVERRIDE { return "SurvivorCombatCloseRange"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return nullptr; }

    virtual void OnStart() OVERRIDE
    {
        m_bMovingToRange = false;
        m_Path.Invalidate();
        m_NextRangeCheck.Invalidate();
        m_NextMovingToRange.Invalidate();


        CZMPlayerBot* pOuter = GetOuter();

        if ( !IsMeleeing() )
        {
            if ( pOuter->EquipWeaponOfType( BOTWEPRANGE_CLOSERANGE ) && pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
            {
                return;
            }

            if ( pOuter->EquipWeaponOfType( BOTWEPRANGE_LONGRANGE ) && pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
            {
                return;
            }
        }
        else
        {
            if ( pOuter->EquipWeaponOfType( BOTWEPRANGE_MELEE ) )
            {
                return;
            }
        }

        
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


        if ( !pOuter->HasEquippedWeaponOfType( BOTWEPRANGE_MELEE ) && !pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
        {
            End( "No ammo left!" );
            return;
        }


        auto* pFollow = pOuter->GetFollowTarget();

        if ( pFollow && pFollow->GetAbsOrigin().DistToSqr( pOuter->GetAbsOrigin() ) > (256.0f*256.0f) )
        {
            End( "Follow target was too far away to attack anymore!" );
            return;
        }



        if ( !m_NextRangeCheck.HasStarted() || m_NextRangeCheck.IsElapsed() )
        {
            m_NextRangeCheck.Start( 0.1f );


            if ( !m_bMovingOutOfRange && ShouldMoveBack( pEnemy ) )
            {
                MoveBackFromThreat( pEnemy );
            }
            else if ( !m_bMovingToRange && !IsInRangeToAttack( pEnemy ) && ShouldMoveInRange() )
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


        // TODO: Aim at head?
        vecAimTarget = pEnemy->WorldSpaceCenter();


        pOuter->GetMotor()->FaceTowards( vecAimTarget );


        // Fix this grace stuff as the bot will not aim accurately.
        float grace = IsMeleeing() ? 60.0f : 8.0f;


        if ( IsInRangeToAttack( pEnemy ) && pOuter->GetMotor()->IsFacing( vecAimTarget, grace ) )
        {
            if ( pOuter->GetSenses()->CanSee( vecAimTarget ) && !pOuter->MustStopToShoot() || pOuter->GetLocalVelocity().IsLengthLessThan( 1.0f ) )
            {
                pOuter->PressFire1( 0.1f );
            }
        }
    }

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
    }

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        return IsDone() ? NPCR::RES_NONE : NPCR::RES_NO;
    }

    bool ShouldMoveInRange() const
    {
        if ( m_NextMovingToRange.HasStarted() && !m_NextMovingToRange.IsElapsed() )
            return false;

        auto* pOuter = GetOuter();

        return pOuter->CanAttack();
    }

    bool IsInRangeToAttack( CBaseEntity* pEnemy ) const
    {
        float flDist = GetOuter()->GetMaxAttackDistance();

        return pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() ) < (flDist*flDist);
    }

    bool ShouldMoveBack( CBaseEntity* pEnemy ) const
    {
        float flDistSqr = pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() );

        float flMoveBackRange = IsMeleeing() ? 32.0f : 128.0f;

        if ( flDistSqr > (flMoveBackRange*flMoveBackRange) )
            return false;


        return true;
    }

    void MoveToShootingRange( CBaseEntity* pEnemy )
    {
        CZMPlayerBot* pOuter = GetOuter();

        const Vector vecEnemyPos = pEnemy->GetAbsOrigin();
        const Vector vecMyPos = pOuter->GetPosition();

        Vector dir = (vecMyPos - vecEnemyPos).Normalized();


        float flDist = pOuter->GetOptimalAttackDistance();

        Vector vecTarget = vecEnemyPos + dir * flDist;

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

        m_NextMovingToRange.Start( IsMeleeing() ? 1.5f : 0.4f );
    }
};
