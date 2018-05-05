#include "cbase.h"

#include "npcr_basenpc.h"
#include "npcr_nonplayer.h"
#include "npcr_motor_nonplayer.h"
#include "npcr_path_follow.h"


NPCR::CNonPlayerMotor::CNonPlayerMotor( NPCR::CBaseNonPlayer* pNPC ) : NPCR::CBaseMotor( pNPC )
{
    m_vecAcceleration = vec3_origin;
    m_vecVelocity = vec3_origin;
}

NPCR::CNonPlayerMotor::~CNonPlayerMotor()
{
    
}

NPCR::CBaseNonPlayer* NPCR::CNonPlayerMotor::GetOuter() const
{
    return static_cast<CBaseNonPlayer*>( BaseClass::GetOuter() );
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

    MoveTowards( vecOrigPos + m_vecVelocity * flUpdateInterval );


    // Adjust our velocity based on how much we moved.
    if ( ShouldAdjustVelocity() )
    {
        m_vecVelocity = ( GetNPC()->GetPosition() - vecOrigPos ) / flUpdateInterval;
    }



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

    SetVelocity( m_vecVelocity );



    if ( !pOldGround && pNewGround )
    {
        GetNPC()->OnLandedGround( pNewGround );
    }
    else if ( pOldGround && !pNewGround )
    {
        GetNPC()->OnLeftGround( pOldGround );
    }
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

    if ( pPath && pPath->ShouldDecelerateToGoal() )
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
