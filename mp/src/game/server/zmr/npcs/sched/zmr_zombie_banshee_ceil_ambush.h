#pragma once


#include "npcs/zmr_zombiebase.h"
#include "npcs/zmr_banshee.h"
#include "npcs/zmr_zombieanimstate.h"



extern ConVar zm_sk_banshee_dmg_leap_ceilambush;

extern ConVar zm_sv_banshee_ceilambush_detectrange;
extern ConVar zm_sv_banshee_ceilambush_maxheight;

// ZMRTODO: Change this to take into account the ceiling height and gravity in case the max height is changed.
#define FASTZOMBIE_CLING_JUMPSPEED      700.0f


class BansheeCeilAmbushSched : public NPCR::CSchedule<CZMBaseZombie>
{
private:
    CountdownTimer m_ExpireTimer;
    CountdownTimer m_NextAmbushCheck;
    bool m_bOnCeiling;
    bool m_bInLeap;
    Vector m_vecCeilingPos;
    Vector m_vecStartPos;
    float m_flLeapTowardsYaw;
    bool m_bDidLeapAttack;
public:
    virtual const char* GetName() const OVERRIDE { return "BansheeCeilingAmbush"; }

    virtual CZMBanshee* GetOuter() const OVERRIDE { return static_cast<CZMBanshee*>( CSchedule<CZMBaseZombie>::GetOuter() ); }

    virtual void OnStart() OVERRIDE
    {
        CZMBanshee* pOuter = GetOuter();

        m_bOnCeiling = false;
        m_bInLeap = false;
        m_bDidLeapAttack = false;


        if ( !pOuter->GetMotor()->IsOnGround() )
        {
            End( "Can't jump to ceiling while in air!" );
            return;
        }


        trace_t tr;
        Vector upwards = Vector( 0.0f, 0.0f, zm_sv_banshee_ceilambush_maxheight.GetFloat() );
        UTIL_TraceEntity( pOuter, pOuter->GetPosition(), pOuter->GetPosition() + upwards, MASK_NPCSOLID, &tr );

        // Valid surface? that is, a brush that is not the sky
        if (tr.fraction == 1.0f
        ||  !tr.DidHitWorld()
        ||  tr.surface.flags & SURF_SKY) 
        {
            End( "Couldn't find valid ceiling!" );
            return;
        }

        if ( !IsCeilingFlat( tr.plane.normal ) )
        {
            End( "Ceiling not flat enough!" );
            return;
        }


        Vector vecMyPos = pOuter->GetPosition() + Vector( 0.0f, 0.0f, 1.0f );

        // Lift them off the ground a bit so step trace doesn't hit the floor.
        pOuter->SetAbsOrigin( vecMyPos );

        pOuter->GetMotor()->SetVelocity( Vector( 0.0f, 0.0f, FASTZOMBIE_CLING_JUMPSPEED ) );
        pOuter->GetMotor()->Jump();

        pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_HOP );


        m_vecStartPos = vecMyPos;

        m_ExpireTimer.Start( 1.4f );
    }

    virtual void OnEnd() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

        if ( pOuter->GetMoveType() == MOVETYPE_NONE )
        {
            pOuter->SetMoveType( MOVETYPE_CUSTOM );
        }

        Activity act = pOuter->GetActivity();
        if ( act == ACT_HOVER || act == ACT_HOP || act == ACT_FASTZOMBIE_LEAP_STRIKE )
        {
            pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
        }
    }

    virtual void OnUpdate() OVERRIDE
    {
        if ( (m_ExpireTimer.HasStarted() && m_ExpireTimer.IsElapsed()) )
        {
            End( "Failed to jump to ceiling!" );
            return;
        }


        if ( m_bOnCeiling )
        {
            CZMBaseZombie* pOuter = GetOuter();

            // This makes sure we are properly touching the ceiling.
            if ( pOuter->GetMoveType() != MOVETYPE_NONE )
                pOuter->SetAbsOrigin( m_vecCeilingPos );


            pOuter->GetMotor()->SetVelocity( vec3_origin );
            pOuter->SetMoveType( MOVETYPE_NONE );


            // Check for enemies to target
            if ( !m_NextAmbushCheck.HasStarted() || m_NextAmbushCheck.IsElapsed() )
            {
                m_NextAmbushCheck.Start( 0.2f );


                CBaseEntity* pTarget = GetClingAmbushTarget();
                if ( pTarget )
                {
                    Leap( pTarget );
                    return;
                }
            }
        }
        // Face towards our leap direction
        else if ( m_bInLeap )
        {
            float delta = abs( GetOuter()->GetAbsAngles().y - m_flLeapTowardsYaw );
            if ( delta > 1.0f )
                GetOuter()->GetMotor()->FaceTowards( m_flLeapTowardsYaw );
        }
    }

    virtual void OnContinue() OVERRIDE
    {
        End( "Done leaping." );
    }

    virtual void OnAnimEvent( animevent_t* pEvent ) OVERRIDE
    {
    }

    virtual void OnAnimActivityInterrupted( Activity newActivity ) OVERRIDE
    {
        if ( newActivity == ACT_HOVER )
            return;

        if ( newActivity == ACT_FASTZOMBIE_LEAP_STRIKE )
            return;


        TryEnd( "Banshee ceiling ambush was interrupted by another activity!" );
    }

    virtual void OnLandedGround( CBaseEntity* pEnt ) OVERRIDE
    {
        if ( m_bInLeap )
        {
            GetOuter()->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );

            TryEnd( "Finished leap successfully!" );
            return;
        }


        TryEnd( "We landed on ground, wat?" );
    }

    virtual void OnTouch( CBaseEntity* pEnt, trace_t* tr ) OVERRIDE
    {
        // Jumping to the ceiling
        if ( !m_bOnCeiling && !m_bInLeap )
        {
            // We presumably touched the ceiling.
            if ( pEnt->IsWorld() && IsCeilingFlat( tr->plane.normal ) )
            {
                // Sets banshee upside down.
                GetOuter()->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_HOVER );

                m_bOnCeiling = true;
                m_vecCeilingPos = tr->endpos;
                m_ExpireTimer.Invalidate();
            }
        }
        // Jumping towards an enemy
        else if ( m_bInLeap )
        {
            CZMBanshee* pOuter = GetOuter();
            float minspd = CZMBanshee::GetMinLeapAttackSpeed();

            // Do some leap damage
            if (!m_bDidLeapAttack
            &&  pOuter->GetVel().LengthSqr() > (minspd*minspd)
            &&  pEnt && pOuter->IsEnemy( pEnt ))
            {
                Vector fwd;
                AngleVectors( pOuter->GetAngles(), &fwd );
                QAngle angPunch( 20.0f, random->RandomInt( -5.0f, 5.0f ), random->RandomInt( -5.0f, 5.0f ) );
                Vector vecPunchVel = fwd * 500.0f;
    
                // Do more damage than normal leap
                float damage = zm_sk_banshee_dmg_leap_ceilambush.GetFloat();

                bool bHit = pOuter->LeapAttack( angPunch, vecPunchVel, damage );

                m_bDidLeapAttack = bHit;
            }
        }
    }

    virtual void OnCommanded( ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
    {
        if ( com == COMMAND_CEILINGAMBUSH )
        {
            GetOuter()->GetCommandQueue()->RemoveCommand( COMMAND_CEILINGAMBUSH );
        }
        else
        {
            TryEnd( "We were commanded to do something else!" );
        }
    }

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return m_bInLeap ? NPCR::RES_YES : NPCR::RES_NONE;
    }

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        return NPCR::RES_NO;
    }

    bool IsCeilingFlat( const Vector& plane )
    {
        const Vector flat = Vector( 0.0f, 0.0f, -1.0f );

        return DotProductAbs( plane, flat ) > 0.95f;
    }

    CBaseEntity* GetClingAmbushTarget()
    {
        CZMBanshee* pOuter = GetOuter();

        CBaseEntity* pNearest = nullptr;
        float nearest_dist = 0.0f;
        CBaseEntity* pList[32];

        Vector vecMyPos = pOuter->GetAbsOrigin();

            
        int count = UTIL_EntitiesInSphere( pList, ARRAYSIZE( pList ), m_vecStartPos, zm_sv_banshee_ceilambush_detectrange.GetFloat(), FL_CLIENT );


        for ( int i = 0; i < count; i++ )
        {
            if ( !pList[i] )
                continue;


            CBaseEntity* pLoop = pList[i];
            if ( pLoop->GetTeamNumber() != ZMTEAM_HUMAN || !pLoop->IsAlive() )
                continue;



            if ( !pOuter->GetSenses()->HasLOS( pLoop->WorldSpaceCenter() ) )
                continue;

            float current_dist = pLoop->GetAbsOrigin().DistToSqr( vecMyPos );
            if ( !pNearest || nearest_dist > current_dist )
            {
                pNearest = pList[i];
                nearest_dist = current_dist;
            }
        }

        return pNearest;
    }

    void Leap( CBaseEntity* pTarget )
    {
        CZMBanshee* pOuter = GetOuter();

        if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_FASTZOMBIE_LEAP_STRIKE ) )
        {
            End( "Couldn't start the leap strike activity!" );
            return;
        }

        // Start moving again
        pOuter->SetMoveType( MOVETYPE_CUSTOM );
        m_bInLeap = true;
        m_bOnCeiling = false;


        pOuter->AcquireEnemy( pTarget );

        // Leap towards the enemy
        pOuter->LeapAttackSound();

        Vector vecTarget = pTarget->WorldSpaceCenter();
        Vector vecMyPos = pOuter->WorldSpaceCenter();
        Vector vecJump = pTarget->WorldSpaceCenter() - pOuter->WorldSpaceCenter();
        vecJump *= 2.0f;
        vecJump.z = vecTarget.z - vecMyPos.z;

        float maxspd = pOuter->GetMaxLeapSpeed();
        float distance = vecJump.Length();
        if ( distance > 0.0f && distance > maxspd )
        {
            vecJump *= (maxspd / distance);
        }

        pOuter->GetMotor()->SetVelocity( vecJump );

        m_flLeapTowardsYaw = atan2f( vecJump.y, vecJump.x );
        pOuter->GetMotor()->FaceTowards( m_flLeapTowardsYaw );
    }
};
