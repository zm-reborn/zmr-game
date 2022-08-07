#include "cbase.h"
#include "npcevent.h"

#include "npcs/zmr_zombieanimstate.h"
#include "zmr_zombie_banshee_leap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CBansheeLeapSchedule::OnStart()
{
    CZMBanshee* pOuter = GetOuter();

    m_bInLeap = false;
    //m_iLandAct = ACT_INVALID;
    m_bLanded = false;
    m_FinishTimer.Invalidate();
    m_ExpireTimer.Start( 5.0f );
    m_bDidLeapAttack = false;


    CBaseEntity* pEnemy = pOuter->GetEnemy();
    if ( !pEnemy )
    {
        End( "No enemy to leap towards!" );
        return;
    }

    if ( !pOuter->GetMotor()->IsOnGround() )
    {
        End( "We're not on ground!" );
        return;
    }

    pOuter->SetNextLeapAttack( gpGlobals->curtime + 4.0f );
}

void CBansheeLeapSchedule::OnUpdate()
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
        //if ( m_iLandAct != ACT_INVALID )
        if ( !m_bLanded )
        {
            CBaseEntity* pEnemy = GetOuter()->GetEnemy();

            if ( pEnemy )
                GetOuter()->GetMotor()->FaceTowards( pEnemy->WorldSpaceCenter() );
        }
    }
}

void CBansheeLeapSchedule::OnAnimEvent( animevent_t* pEvent )
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
            TryEnd( "Failed to start leap movement!" );
            return;
        }

        return;
    }
}

void CBansheeLeapSchedule::OnLandedGround( CBaseEntity* pGround )
{
    auto* pOuter = GetOuter();

    //if ( pOuter->GetActivity() != m_iLandAct )
    if ( !m_bLanded )
    {
        /*
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
        */
        pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );

        m_bLanded = true;
    }

    m_FinishTimer.Start( 0.35f );
}

void CBansheeLeapSchedule::OnTouch( CBaseEntity* pEnt, trace_t* pTrace )
{
    float minspd = CZMBanshee::GetMinLeapAttackSpeed();
    if (!m_bDidLeapAttack
    &&  GetOuter()->GetVel().LengthSqr() > (minspd*minspd)
    &&  pEnt )
    {
        //
        // We're leaping and hit something.
        //
        CZMBanshee* pOuter = GetOuter();

        // Force players to drop this prop!
        auto* pPhys = pEnt->VPhysicsGetObject();
        if ( pPhys && pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
        {
            auto* pPlayer = ToZMPlayer( pEnt->GetOwnerEntity() );
        
            if ( pPlayer )
            {
                pPlayer->ForceDropOfCarriedPhysObjects( pEnt );
            }
        }

        // Attack enemy.
        if ( pOuter->IsEnemy( pEnt ) )
        {
            Vector fwd;
            AngleVectors( pOuter->GetAngles(), &fwd );
            QAngle angPunch( 15.0f, random->RandomInt( -5.0f, 5.0f ), random->RandomInt( -5.0f, 5.0f ) );
            Vector vecPunchVel = fwd * 500.0f;


            float damage = zm_sk_banshee_dmg_leap.GetFloat();
    
            bool bHit = pOuter->LeapAttack( angPunch, vecPunchVel, damage );

            m_bDidLeapAttack = bHit;
        }

    }
}

void CBansheeLeapSchedule::OnAnimActivityInterrupted( Activity newActivity )
{
    if ( newActivity == ACT_RANGE_ATTACK1 )
        return;

    if ( newActivity == ACT_FASTZOMBIE_LEAP_STRIKE )
        return;

    //if ( newActivity == m_iLandAct )
    //    return;

    if ( newActivity == ACT_FASTZOMBIE_FRENZY )
        return;


    TryEnd( "Banshee leap was interrupted by another activity!" );
}

void CBansheeLeapSchedule::OnAnimActivityFinished( Activity completedActivity )
{
    if ( completedActivity == ACT_RANGE_ATTACK1 )
    {
        auto* pOuter = GetOuter();

        if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_BANSHEEANIM, ACT_FASTZOMBIE_LEAP_STRIKE ) )
        {
            TryEnd( "Couldn't start the leap strike activity!" );
        }

        return;
    }
}

void CBansheeLeapSchedule::OnEnd()
{
    // This activity loops, so we'll have to manually stop it.
    if ( GetOuter()->GetActivity() == ACT_FASTZOMBIE_LEAP_STRIKE )
        GetOuter()->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
}

NPCR::QueryResult_t CBansheeLeapSchedule::IsBusy() const
{
    return NPCR::RES_YES;
}

void CBansheeLeapSchedule::OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com )
{
    TryEnd( "We were commanded to do something else!" );
}

void CBansheeLeapSchedule::OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com )
{
    // ZM wants us to do something else.
    if ( m_bInLeap )
        return;


    if ( pPlayer )
    {
        // Check if ZM wants for our attack to be interrupted.
        int flags = ToZMPlayer( pPlayer )->GetZMCommandInterruptFlags();

        if ( !(flags & ZCO_ATTACK) )
            return;
    }


    TryEnd( "We were commanded to do something else!" );
}

void CBansheeLeapSchedule::DoLeapStart()
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

bool CBansheeLeapSchedule::DoLeap()
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

    float speed = sqrtf( 2.0f * gravity * MAX( 1.0f, height ) );

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