#include "cbase.h"

#include "zmr_survivor_attack_longrange.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CSurvivorAttackLongRangeSchedule::CSurvivorAttackLongRangeSchedule()
{
    m_pAttackCloseRangeSched = new CSurvivorAttackCloseRangeSchedule;
}

CSurvivorAttackLongRangeSchedule::~CSurvivorAttackLongRangeSchedule()
{
    delete m_pAttackCloseRangeSched;
}

void CSurvivorAttackLongRangeSchedule::OnStart()
{
    CZMPlayerBot* pOuter = GetOuter();

    if ( !pOuter->EquipWeaponOfType( BOTWEPRANGE_LONGRANGE ) )
    {
        End( "Couldn't equip any long range weapons!" );
        return;
    }
}

void CSurvivorAttackLongRangeSchedule::OnContinue()
{
    End( "Done." );
}

void CSurvivorAttackLongRangeSchedule::OnUpdate()
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


    if ( pEnemy->GetAbsOrigin().DistTo( pOuter->GetPosition() ) < 128.0f )
    {
        m_pAttackCloseRangeSched->SetAttackTarget( pEnemy );
        Intercept( m_pAttackCloseRangeSched, "Enemy too close!" );
        return;
    }


    Vector vecAimTarget;


    vecAimTarget = pEnemy->WorldSpaceCenter();

    pOuter->GetMotor()->FaceTowards( vecAimTarget );

    if ( pOuter->GetMotor()->IsFacing( vecAimTarget, 1.0f ) && pOuter->GetSenses()->CanSee( vecAimTarget ) )
    {
        pOuter->PressFire1( 0.1f );
    }
}

NPCR::QueryResult_t CSurvivorAttackLongRangeSchedule::IsBusy() const
{
    return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
}

NPCR::QueryResult_t CSurvivorAttackLongRangeSchedule::ShouldChase( CBaseEntity* pEnemy ) const
{
    return IsDone() ? NPCR::RES_NONE : NPCR::RES_NO;
}
