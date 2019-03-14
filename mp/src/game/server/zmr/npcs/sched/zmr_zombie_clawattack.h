#pragma once


#include "zmr/npcs/zmr_zombiebase.h"

class ClawAttackSched : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    CountdownTimer m_FinishTimer;
    Vector m_vecLastFacing;
    bool m_bDidAttack;
public:
    ClawAttackSched()
    {

    }
    ~ClawAttackSched()
    {
            
    }

    virtual const char* GetName() const OVERRIDE { return "ZombieClawAttack"; }


    virtual void OnStart() OVERRIDE
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

        m_FinishTimer.Start( wait );
        m_vecLastFacing = pEnemy->WorldSpaceCenter();
    }

    virtual void OnUpdate() OVERRIDE
    {
        if ( m_FinishTimer.IsElapsed() )
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

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE
    {
        if ( GetOuter()->GetActivity() != ACT_MELEE_ATTACK1 )
            return;


        TryEnd( "Claw attack was interrupted!" );
    }

    // Wait for us to finish before doing something else.
    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return NPCR::RES_YES;
    }

    virtual void OnAttacked() OVERRIDE
    {
        m_bDidAttack = true;
    }

    virtual void OnEnd() OVERRIDE
    {
        auto* pOuter = GetOuter();
        if ( pOuter->GetActivity() == ACT_MELEE_ATTACK1 && !pOuter->IsSequenceFinished() )
        {
            pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
        }
    }

    virtual void OnCommanded( ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
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
};
