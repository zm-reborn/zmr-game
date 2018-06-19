#pragma once

#include "soundent.h"

#include "zmr_zombie_chase.h"


class CombatSchedule : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    ChaseSched* m_pChaseSched;

    Vector m_vecFaceTowards;
    CountdownTimer m_FaceTimer;
    CountdownTimer m_NextEnemyScan;
public:
    CombatSchedule()
    {
        m_vecFaceTowards = vec3_origin;

        m_pChaseSched = new ChaseSched;
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
        m_NextEnemyScan.Invalidate();
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


        // Face towards possible danger.
        bool bCanMove = (pOuter->IsBusy() != NPCR::RES_YES && !pOuter->GetMotor()->IsMoving());

        if ( bCanMove && m_FaceTimer.HasStarted() && !m_FaceTimer.IsElapsed() )
        {
            pOuter->GetMotor()->FaceTowards( m_vecFaceTowards );

            if ( pOuter->GetMotor()->IsFacing( m_vecFaceTowards, 10.0f ) )
            {
                m_FaceTimer.Invalidate();
            }
        }


        // Check for any enemies we might see.
        if ( !m_NextEnemyScan.HasStarted() || m_NextEnemyScan.IsElapsed() )
        {
            CBaseEntity* pClosest = pOuter->GetSenses()->GetClosestEntity();
            CBaseEntity* pOldEnemy = pOuter->GetEnemy();

            // Make sure our current enemy is valid.
            if ( pOldEnemy && !pOuter->IsEnemy( pOldEnemy ) )
            {
                pOuter->LostEnemy();
                pOldEnemy = nullptr;
            }

            CBaseEntity* pEnemy = pClosest && pOuter->IsEnemy( pClosest ) ? pClosest : pOldEnemy;

            if ( pEnemy && pOuter->ShouldChase( pEnemy ) != NPCR::RES_NO )
            {
                pOuter->AcquireEnemy( pEnemy );

                if ( !pOldEnemy )
                {
                    pOuter->AlertSound();

                    pOuter->RemoveSpawnFlags( SF_NPC_GAG );
                }

                Intercept( m_pChaseSched, "We see a potential enemy!" );
                return;
            }

            m_NextEnemyScan.Start( 0.2f );
        }
    }

    virtual void OnSightGained( CBaseEntity* pEnt ) OVERRIDE
    {
        if ( GetOuter()->IsEnemy( pEnt ) )
        {
            m_NextEnemyScan.Invalidate();
            return;
        }
    }

    virtual void OnDamaged( const CTakeDamageInfo& info ) OVERRIDE
    {
        CBaseEntity* pAttacker = info.GetAttacker();
        if ( pAttacker && GetOuter()->IsEnemy( pAttacker ) )
        {
            m_vecFaceTowards = pAttacker->GetAbsOrigin();
            m_FaceTimer.Start( 2.0f );
        }
    }

    virtual void OnHeardSound( CSound* pSound ) OVERRIDE
    {
        // Try to look for any enemies.
        if ( !GetOuter()->GetEnemy() && !m_FaceTimer.HasStarted() && pSound->IsSoundType( SOUND_COMBAT | SOUND_PLAYER ) )
        {
            m_vecFaceTowards = pSound->GetSoundReactOrigin();
            m_FaceTimer.Start( 2.0f );
        }
    }
};
