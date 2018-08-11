#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_nonplayer.h"
#include "npcr_motor_nonplayer.h"
#include "npcr_path_follow.h"


extern ConVar npcr_debug_navigator;


NPCR::CNonPlayerMotor::CNonPlayerMotor( CNPCRNonPlayer* pNPC ) : NPCR::CBaseMotor( pNPC )
{
    m_vecAcceleration = vec3_origin;
    m_vecVelocity = vec3_origin;
    m_vecLastBaseVelocity = vec3_origin;
}

NPCR::CNonPlayerMotor::~CNonPlayerMotor()
{
    
}

CNPCRNonPlayer* NPCR::CNonPlayerMotor::GetOuter() const
{
    return static_cast<CNPCRNonPlayer*>( BaseClass::GetOuter() );
}


bool NPCR::CNonPlayerMotor::ShouldApplyGroundMove() const
{
    return GetGroundNormal().z > GetSlopeLimit();
}

void NPCR::CNonPlayerMotor::Approach( const Vector& vecDesiredGoal )
{
    FaceTowards( vecDesiredGoal );

    BaseClass::Approach( vecDesiredGoal );
}

void NPCR::CNonPlayerMotor::Update()
{
    if ( GetOuter()->GetMoveType() == MOVETYPE_CUSTOM )
        Move();

    BaseClass::Update();
}

void NPCR::CNonPlayerMotor::Move()
{
    m_vecAcceleration = vec3_origin;

    CBaseEntity* pOldGround = GetGroundEntity();
    bool bWasOnGround = IsOnGround();

    float flUpdateInterval = GetNPC()->GetUpdateInterval();
    

    CBaseEntity* pNewGround = UpdateGround();

    if ( !pOldGround && pNewGround )
    {
        GetNPC()->OnLandedGround( pNewGround );
    }
    else if ( pOldGround && !pNewGround )
    {
        GetNPC()->OnLeftGround( pOldGround );
    }


    bool bIsOnGround = pNewGround != nullptr;

    if ( !bIsOnGround )
    {
        // Apply gravity.
        m_vecAcceleration.z -= GetGravity();
    }

    Vector vecOrigPos = GetNPC()->GetPosition();


    bool bDidMove = GroundMove();
    if ( bDidMove )
    {
        UpdateMoving();
    }
    else
    {
        // We no longer need to decelerate when moving.
        m_bDoDecelerate = false;
    }


    // Apply friction.
    if ( bIsOnGround )
    {
        if ( bDidMove )
        {
            // Apply special friction when moving.
            Vector right( m_vecMoveDir.y, -m_vecMoveDir.x, m_vecMoveDir.z );
            Vector rightVel = DotProduct( m_vecVelocity, right ) * right;
            Vector swFriction = -GetFrictionSideways() * rightVel;

            if ( npcr_debug_navigator.GetBool() )
            {
                Vector pos = GetNPC()->GetPosition();
                pos.z += 100.0f;
                NDebugOverlay::Line( pos, pos + swFriction, 255, 0, 0, true, 0.1f );
            }

            m_vecAcceleration.x += swFriction.x;
            m_vecAcceleration.y += swFriction.y;
        }
        else
        {
            float friction = -GetFriction();
            m_vecAcceleration.x += friction * m_vecVelocity.x;
            m_vecAcceleration.y += friction * m_vecVelocity.y;
        }
    }


    // Do the actual moving.


    m_vecVelocity += m_vecAcceleration * flUpdateInterval;



    // Apply base velocity (from trigger_push, etc.)
    Vector vecBaseVel = GetOuter()->GetBaseVelocity();
    bool bApplyBaseVel = ( vecBaseVel.LengthSqr() > 1.0f ) ? true : false;
    
    // If we're currently applying the base velocity, we simply substract it after moving.
    // If we're done with base velocity, we need to apply it to the velocity as to keep the velocity.
    if ( bApplyBaseVel )
    {
        m_vecVelocity += vecBaseVel;
        m_vecLastBaseVelocity = vecBaseVel;
    }
    else
    {
        m_vecVelocity += m_vecLastBaseVelocity;
        m_vecLastBaseVelocity = vec3_origin;
    }

    MoveTowards( vecOrigPos + m_vecVelocity * flUpdateInterval );

    if ( bApplyBaseVel )
    {
        m_vecVelocity -= vecBaseVel;
    }

    // Adjust our velocity based on how much we moved.
    if ( ShouldAdjustVelocity() && !bApplyBaseVel )
    {
        m_vecVelocity = ( GetNPC()->GetPosition() - vecOrigPos ) / flUpdateInterval;
    }

    // Base velocity needs to be substracted.
    //if ( bApplyBaseVel )
    //{
    //    m_vecVelocity -= vecBaseVel;
    //}



    if ( bIsOnGround )
    {
        float flMoveSpeed = GetMovementSpeed();

        if ( IsAttemptingToMove() && m_vecVelocity.IsLengthGreaterThan( flMoveSpeed ) )
        {
            m_vecVelocity.NormalizeInPlace();
            m_vecVelocity *= flMoveSpeed;
        }

        if ( !bWasOnGround )
        {
            m_vecVelocity.z = 0.0f;
        }
    }


    // Velocity needs to be clamped.
    // See k_flMaxVelocity
    const float flAbsMaxVel = 2000.0f;
    if (abs( m_vecVelocity.x ) > flAbsMaxVel
    ||  abs( m_vecVelocity.y ) > flAbsMaxVel
    ||  abs( m_vecVelocity.z ) > flAbsMaxVel)
    {
        m_vecVelocity.NormalizeInPlace();
        m_vecVelocity *= flAbsMaxVel;
    }


    SetVelocity( m_vecVelocity );
}

bool NPCR::CNonPlayerMotor::GroundMove()
{
    if ( !ShouldApplyGroundMove() )
    {
        return false;
    }

    // We haven't been signaled to move!
    if ( !IsAttemptingToMove() )
    {
        return false;
    }

    // Set the acceleration according to our direction we want to move on the ground.
    m_vecMoveDir = m_vecDesiredMoveDir;
    float distToGoal = m_vecMoveDir.NormalizeInPlace();


    Vector left( -m_vecMoveDir.y, m_vecMoveDir.x, 0.0f );
    m_vecMoveDir = CrossProduct( left, GetGroundNormal() );
    m_vecMoveDir.NormalizeInPlace();

    if ( npcr_debug_navigator.GetBool() )
    {
        Vector pos = GetNPC()->GetPosition();
        pos.z += 100.0f;
        NDebugOverlay::Line( pos, pos + m_vecMoveDir * 16.0f, 0, 255, 0, true, 0.1f );
    }


    float flMaxSpd = GetMovementSpeed();
    if ( flMaxSpd <= 0.0f ) return false;


    // Accelerate
    float fwdSpd = DotProduct( m_vecVelocity, m_vecMoveDir );
    if ( fwdSpd < flMaxSpd )
    {
        float ratio = 1.0f - ( (fwdSpd <= 0.0f) ? 0.0f : (fwdSpd / flMaxSpd) );
        m_vecAcceleration += ratio * GetMaxAcceleration() * m_vecMoveDir;
    }


    

    // Only allow deceleration after we've gone near full speed.
    if ( fwdSpd > (flMaxSpd*0.9f) )
    {
        m_bDoDecelerate = true;
    }

    // Decelerate when we are nearing the goal.
    auto* pPath = GetNPC()->GetCurrentPath();

    if ( m_bDoDecelerate && pPath && pPath->ShouldDecelerateToGoal() )
    {
        const float flMinDecDist = pPath->GetGoalTolerance() + 16.0f;

        const float flMinSpeed = 16.0f;


        float decDist = flMaxSpd * 0.5f;
        if ( decDist < flMinDecDist )
            decDist = flMinDecDist;

        if ( distToGoal < decDist && fwdSpd > flMinSpeed )
        {
            float ratio = 1.0f - distToGoal / decDist;

            m_vecAcceleration += ratio * -GetMaxDeceleration() * m_vecMoveDir;
        }
    }

    return true;
}

float NPCR::CNonPlayerMotor::GetMovementSpeed() const
{
    float groundspd = GetOuter()->GetMoveActivityMovementSpeed();
    Assert( groundspd > 0.0f );

    return groundspd;
}
