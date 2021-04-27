#include "cbase.h"

#include "npcr_motor.h"
#include "zmr_survivor_attack_closerange.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CSurvivorAttackCloseRangeSchedule::CSurvivorAttackCloseRangeSchedule()
{
    m_bAllowMelee = false;
}

CSurvivorAttackCloseRangeSchedule::~CSurvivorAttackCloseRangeSchedule()
{
}

void CSurvivorAttackCloseRangeSchedule::OnStart()
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

void CSurvivorAttackCloseRangeSchedule::OnContinue()
{
    End( "Done." );
}

void CSurvivorAttackCloseRangeSchedule::OnUpdate()
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
        if (pOuter->GetSenses()->CanSee( vecAimTarget ) &&
            (!pOuter->MustStopToShoot() || pOuter->GetLocalVelocity().IsLengthLessThan( 1.0f )))
        {
            pOuter->PressFire1( 0.1f );
        }
    }
}

NPCR::QueryResult_t CSurvivorAttackCloseRangeSchedule::IsBusy() const
{
    return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
}

NPCR::QueryResult_t CSurvivorAttackCloseRangeSchedule::ShouldChase( CBaseEntity* pEnemy ) const
{
    return IsDone() ? NPCR::RES_NONE : NPCR::RES_NO;
}

bool CSurvivorAttackCloseRangeSchedule::ShouldMoveInRange() const
{
    if ( m_NextMovingToRange.HasStarted() && !m_NextMovingToRange.IsElapsed() )
        return false;

    auto* pOuter = GetOuter();

    return pOuter->CanAttack();
}

bool CSurvivorAttackCloseRangeSchedule::IsInRangeToAttack( CBaseEntity* pEnemy ) const
{
    float flDist = GetOuter()->GetMaxAttackDistance();

    return pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() ) < (flDist*flDist);
}

bool CSurvivorAttackCloseRangeSchedule::ShouldMoveBack( CBaseEntity* pEnemy ) const
{
    float flDistSqr = pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() );

    float flMoveBackRange = IsMeleeing() ? 32.0f : 128.0f;

    if ( flDistSqr > (flMoveBackRange*flMoveBackRange) )
        return false;


    return true;
}

void CSurvivorAttackCloseRangeSchedule::MoveToShootingRange( CBaseEntity* pEnemy )
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

void CSurvivorAttackCloseRangeSchedule::MoveBackFromThreat( CBaseEntity* pEnemy )
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