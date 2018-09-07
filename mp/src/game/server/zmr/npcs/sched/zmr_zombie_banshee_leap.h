#pragma once


#include "zmr/npcs/zmr_zombiebase.h"
#include "zmr/npcs/zmr_banshee.h"
#include "zmr/npcs/zmr_zombieanimstate.h"


extern ConVar zm_sk_banshee_dmg_leap;


class BansheeLeapSched : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    bool m_bInLeap;
    Activity m_iLandAct;
    CountdownTimer m_FinishTimer;
    CountdownTimer m_ExpireTimer;
    bool m_bDidLeapAttack;
public:
    virtual const char* GetName() const OVERRIDE { return "BansheeLeap"; }

    virtual CZMBanshee* GetOuter() const OVERRIDE { return static_cast<CZMBanshee*>( CSchedule<CZMBaseZombie>::GetOuter() ); }

    virtual void OnStart() OVERRIDE
    {
        CZMBanshee* pOuter = GetOuter();

        m_bInLeap = false;
        m_iLandAct = ACT_INVALID;
        m_FinishTimer.Invalidate();
        m_ExpireTimer.Start( 5.0f );
        m_bDidLeapAttack = false;


        CBaseEntity* pEnemy = pOuter->GetEnemy();
        if ( !pEnemy )
        {
            End( "No enemy to leap towards!" );
            return;
        }


        pOuter->SetNextLeapAttack( gpGlobals->curtime + 4.0f );
    }

    virtual void OnUpdate() OVERRIDE
    {
        if ( m_ExpireTimer.IsElapsed() )
        {
            End( "Leap expired!" );
            return;
        }

        if ( m_FinishTimer.HasStarted() && m_FinishTimer.IsElapsed() )
        {
            End( "Successfully finished leap attack!" );
            return;
        }


        if ( !m_bInLeap )
        {
            DoLeapStart();
        }
        else
        {
            // When landing, keep facing the enemy.
            if ( m_iLandAct != ACT_INVALID )
            {
                CBaseEntity* pEnemy = GetOuter()->GetEnemy();

                if ( pEnemy )
                    GetOuter()->GetMotor()->FaceTowards( pEnemy->WorldSpaceCenter() );
            }
        }
    }

    virtual void OnAnimEvent( animevent_t* pEvent ) OVERRIDE
    {
        if ( pEvent->event == AE_FASTZOMBIE_LEAP )
        {
            GetOuter()->HandledAnimEvent();
            

            if ( DoLeap() )
            {
                m_bInLeap = true;
            }
            else
            {
                End( "Failed to start leap movement!" );
                return;
            }

            return;
        }
    }

    virtual void OnLandedGround( CBaseEntity* pGround ) OVERRIDE
    {
        CZMBanshee* pOuter = GetOuter();

        if ( pOuter->GetActivity() != m_iLandAct )
        {
            m_iLandAct = ACT_FASTZOMBIE_LAND_RIGHT;

            // See which way we're gonna land.
            CBaseEntity* pEnemy = GetOuter()->GetEnemy();
            if ( pEnemy )
            {
                Vector dirToEnemy = pEnemy->GetAbsOrigin() - GetOuter()->GetAbsOrigin();
                float delta = atan2( dirToEnemy.y, dirToEnemy.x ) - GetOuter()->GetAbsAngles().y;
                if ( delta > 0.0f )
                    m_iLandAct = ACT_FASTZOMBIE_LAND_LEFT;
            }

            pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, m_iLandAct );
        }

        m_FinishTimer.Start( 0.35f );
    }

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* pTrace ) OVERRIDE
    {
        float minspd = CZMBanshee::GetMinLeapAttackSpeed();
        if (!m_bDidLeapAttack
        &&  GetOuter()->GetVel().LengthSqr() > (minspd*minspd)
        &&  pEnt && GetOuter()->IsEnemy( pEnt ))
        {
            CZMBanshee* pOuter = GetOuter();

            Vector fwd;
            AngleVectors( pOuter->GetAngles(), &fwd );
            QAngle angPunch( 15.0f, random->RandomInt( -5.0f, 5.0f ), random->RandomInt( -5.0f, 5.0f ) );
            Vector vecPunchVel = fwd * 500.0f;


            float damage = zm_sk_banshee_dmg_leap.GetFloat();
    
            bool bHit = pOuter->LeapAttack( angPunch, vecPunchVel, damage );

            m_bDidLeapAttack = bHit;
        }
    }

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE
    {
        if ( IsDone() )
            return;

        if ( newActivity == ACT_RANGE_ATTACK1 )
            return;

        if ( newActivity == ACT_FASTZOMBIE_LEAP_STRIKE )
            return;

        if ( newActivity == m_iLandAct )
            return;

        if ( newActivity == ACT_FASTZOMBIE_FRENZY )
            return;


        End( "Banshee leap was interrupted by another activity!" );
    }

    virtual void OnAnimActivityFinished( Activity completedActivity ) OVERRIDE
    {
        if ( completedActivity == ACT_RANGE_ATTACK1 )
        {
            CZMBanshee* pOuter = GetOuter();

            if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_FASTZOMBIE_LEAP_STRIKE ) )
            {
                End( "Couldn't start the leap strike activity!" );
            }

            return;
        }
    }

    virtual void OnEnd() OVERRIDE
    {
        // This activity loops, so we'll have to manually stop it.
        if ( GetOuter()->GetActivity() == ACT_FASTZOMBIE_LEAP_STRIKE )
            GetOuter()->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
    }

    virtual void OnCommanded( ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return (!IsDone() && m_bInLeap) ? NPCR::RES_YES : NPCR::RES_NONE;
    }

    void DoLeapStart()
    {
        CZMBanshee* pOuter = GetOuter();

        CBaseEntity* pEnemy = pOuter->GetEnemy();
        if ( !pEnemy )
        {
            End( "No enemy to leap towards!" );
            return;
        }


        Vector vecEnemyPos = pEnemy->GetAbsOrigin();

        pOuter->GetMotor()->FaceTowards( vecEnemyPos );


        if ( pOuter->GetActivity() != ACT_RANGE_ATTACK1 )
        {
            Vector vecMyDir = pOuter->EyeDirection3D();
            Vector vecRealDir = ( vecEnemyPos - pOuter->GetAbsOrigin() ).Normalized();

            // We're close enough.
            if ( pOuter->GetMotor()->IsFacing( vecEnemyPos, 10.0f ) )
            {
                if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_RANGE_ATTACK1 ) )
                {
                    End( "Couldn't start leap start activity!" );
                    return;
                }
            }
        }
    }

    bool DoLeap()
    {
        CZMBanshee* pOuter = GetOuter();

        CBaseEntity* pEnemy = pOuter->GetEnemy();
        if ( !pEnemy )
            return false;


        pOuter->LeapAttackSound();

        Vector vecJump;

        Vector vecEnemyPos = pEnemy->WorldSpaceCenter() + Vector( 0.0f, 0.0f, 8.0f ); // Aim a bit higher
        Vector vecMyPos = pOuter->GetAbsOrigin();

        pOuter->SetAbsOrigin( vecMyPos + Vector( 0, 0, 1 ) );

        float gravity = pOuter->GetMotor()->GetGravity();
        if ( gravity <= 1 )
        {
            gravity = 1;
        }

        //
        // How fast does the zombie need to travel to reach my enemy's position given gravity?
        //
        float height = ( vecEnemyPos.z - vecMyPos.z );

        float speed = sqrt( 2 * gravity * MAX( 1.0f, height ) );

        //
        // Scale the sideways velocity to get there at the right time
        //
        vecJump = vecEnemyPos - vecMyPos;
        vecJump = vecJump / (speed / gravity);
        vecJump.z = height >= 0.0f ? speed : -speed; // We may be going downwards.
            
        float maxspd = pOuter->GetMaxLeapSpeed();
        float distance = vecJump.Length();
        if ( distance > 0.0f && distance > maxspd )
        {
            vecJump *= (maxspd / distance);
        }


        pOuter->GetMotor()->SetVelocity( vecJump );
        pOuter->GetMotor()->Jump();

        return true;
    }
};
