#include "cbase.h"

#include "npcr_motor_player.h"


extern ConVar sv_stepsize;


NPCR::CPlayerMotor::CPlayerMotor( CPlayerCmdHandler* pNPC ) : CBaseMotor( pNPC )
{
}

NPCR::CPlayerMotor::~CPlayerMotor()
{
}

float NPCR::CPlayerMotor::GetStepHeight() const
{
    return sv_stepsize.GetFloat();
}

// Do a smooth transition to goal
float NPCR::CPlayerMotor::GetPitchRate( float delta ) const
{
    return RemapValClamped( fabsf( delta ), 0.0f, 60.0f, 2.0f, 200.0f );
}

float NPCR::CPlayerMotor::GetYawRate( float delta ) const
{
    return RemapValClamped( fabsf( delta ), 0.0f, 60.0f, 2.0f, 200.0f );
}

bool NPCR::CPlayerMotor::ShouldDoFullMove() const
{
    // We're in water, we can go up and down.
    return GetOuter()->GetWaterLevel() >= 2;
}

void NPCR::CPlayerMotor::Update()
{
    Move();


    BaseClass::Update();
}

void NPCR::CPlayerMotor::Move()
{
    m_vecMoveDir = m_vecDesiredMoveDir;

    bool bFullMove = ShouldDoFullMove();
    QAngle ang = GetNPC()->GetEyeAngles();
    ang.z = 0.0f;
    Vector scale;
    Vector fwd, right, up;

    if ( !bFullMove )
    {
        m_vecMoveDir.z = 0.0f;
        ang.x = 0.0f;
    }

    m_flMoveDist = m_vecMoveDir.NormalizeInPlace();
    
    const float epsilon = 0.1f;
    if ( m_flMoveDist < epsilon )
    {
        return;
    }


    AngleVectors( ang, &fwd, &right, &up );

    scale.x = fwd.Dot( m_vecMoveDir );
    scale.y = right.Dot( m_vecMoveDir );
    scale.z = up.Dot( m_vecMoveDir );

    m_vecMoveDir = scale;
}
