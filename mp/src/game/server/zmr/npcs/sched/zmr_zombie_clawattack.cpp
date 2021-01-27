#include "cbase.h"

#include "npcs/zmr_zombieanimstate.h"
#include "zmr_zombie_clawattack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CZombieClawAttackSched::CZombieClawAttackSched()
{

}

CZombieClawAttackSched::~CZombieClawAttackSched()
{
            
}

void CZombieClawAttackSched::OnStart()
{
    m_bDidAttack = false;


    CZMBaseZombie* pOuter = GetOuter();

    CBaseEntity* pEnemy = pOuter->GetEnemy();
    if ( !pEnemy )
    {
        End( "No enemy to attack!" );
        return;
    }


        
    if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_ATTACK ) )
    {
        End( "Couldn't start claw attack activity!" );
        return;
    }


    float wait = pOuter->SequenceDuration();
    pOuter->SetNextAttack( gpGlobals->curtime + wait );
    //pOuter->SetNextMove( gpGlobals->curtime + wait );

    if ( !pOuter->SequenceLoops() )
    {
        // Make sure we finish.
        m_FinishTimer.Start( wait );
    }
    else
    {
        // We loop, assume our class takes care of this one.
        // But add a sanity check.
        m_FinishTimer.Start( wait + 10.0f );
    }

    m_vecLastFacing = pEnemy->WorldSpaceCenter();
}

void CZombieClawAttackSched::OnUpdate()
{
    if ( m_FinishTimer.HasStarted() && m_FinishTimer.IsElapsed() )
    {
        End( "Successfully finished attacking!" );
        return;
    }


    CZMBaseZombie* pOuter = GetOuter();


    // If we for some reason lose the enemy, keep facing to the old direction.
    CBaseEntity* pEnemy = pOuter->GetEnemy();
    Vector face = pEnemy ? pEnemy->WorldSpaceCenter() : m_vecLastFacing;

    pOuter->GetMotor()->FaceTowards( face );

    m_vecLastFacing = face;
}

//void CZombieClawAttackSched::OnAnimActivityFinished( Activity completedActivity )
//{
//    auto* pOuter = GetOuter();
//    if ( completedActivity == ACT_MELEE_ATTACK1 && !pOuter->SequenceLoops() )
//    {
//    }
//}

void CZombieClawAttackSched::OnAnimActivityInterrupted( Activity newActivity )
{
    if ( GetOuter()->GetActivity() != ACT_MELEE_ATTACK1 )
        return;


    TryEnd( "Claw attack was interrupted!" );
}

// Wait for us to finish before doing something else.
NPCR::QueryResult_t CZombieClawAttackSched::IsBusy() const
{
    return NPCR::RES_YES;
}

NPCR::QueryResult_t CZombieClawAttackSched::ShouldChase( CBaseEntity* pEnemy ) const
{
    return NPCR::RES_NO;
}

void CZombieClawAttackSched::OnAttacked()
{
    m_bDidAttack = true;
}

void CZombieClawAttackSched::OnEnd()
{
    auto* pOuter = GetOuter();
    if ( pOuter->GetActivity() == ACT_MELEE_ATTACK1 && !pOuter->IsSequenceFinished() )
    {
        pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
    }
}

void CZombieClawAttackSched::OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com )
{
    TryEnd( "We were commanded to do something else!" );
}

void CZombieClawAttackSched::OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com )
{
    // ZM wants us to do something else.

    if ( !m_bDidAttack && pPlayer )
    {
        // Check if ZM wants for our attack to be interrupted.
        int flags = ToZMPlayer( pPlayer )->GetZMCommandInterruptFlags();

        if ( !(flags & ZCO_ATTACK) )
            return;
    }


    TryEnd( "We were commanded to do something else!" );
}