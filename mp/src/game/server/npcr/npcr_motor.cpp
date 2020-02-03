#include "cbase.h"

#include "npcr_component.h"
#include "npcr_motor.h"
#include "npcr_nonplayer.h"


ConVar npcr_debug_navigator( "npcr_debug_navigator", "0" );
ConVar npcr_debug_navigator_ground( "npcr_debug_navigator_ground", "0" );

ConVar npcr_motor_jump_groundoffset( "npcr_motor_jump_groundoffset", "2" );


class CMoveFilter : public CTraceFilterSimple
{
public:
    CMoveFilter( NPCR::CBaseNPC* pNPC, int collisionGroup ) : CTraceFilterSimple( pNPC->GetCharacter(), collisionGroup )
    {
        m_pNPC = pNPC;
        m_pRootParent = pNPC->GetCharacter()->GetRootMoveParent();
    }

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE
    {
        CBaseEntity* pEnt = EntityFromEntityHandle( pHandleEntity );
        if ( !pEnt )
            return false;

        // We may have something parented to us. Ignore em.
        if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pEnt ) )
            return false;


        NPCR::QueryResult_t res = m_pNPC->ShouldTouch( pEnt );
        if ( res == NPCR::RES_YES )
            return true;
        if ( res == NPCR::RES_NO )
            return false;


        return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
    }

private:
    NPCR::CBaseNPC* m_pNPC;
    CBaseEntity* m_pRootParent;
};


NPCR::CBaseMotor::CBaseMotor( CBaseNPC* pNPC ) : NPCR::CEventListener( pNPC, pNPC )
{
    m_vecMoveDir = vec3_origin;
    m_vecDesiredMoveDir = vec3_origin;
    m_vecGroundNormal = Vector( 0, 0, 1 );
    m_vecLastValidPos = vec3_origin;
    m_bHasValidPos = false;
    m_bForceGravity = false;
    m_flGroundZOffset = 0.0f;

    m_bDoMove = false;

    m_bOnGround = false;
    m_hGroundEnt.Set( nullptr );

    m_bDoStepDownTrace = true;
}

NPCR::CBaseMotor::~CBaseMotor()
{
}

CBaseEntity* NPCR::CBaseMotor::UpdateGround()
{
    m_bForceGravity = false;

    // Check if we're on ground and snap to it if we are.
    // This will become important later.
    // ZMRTODO: Make sure ground check delta is made smaller when jumping so we don't teleport to the ground instantly.
    Vector startPos = GetNPC()->GetPosition();
    float height = GetStepHeight();

    float halfhull = GetHullWidth() / 2.0f;
    Vector mins = Vector( -halfhull, -halfhull, 0.0f );
    Vector maxs = Vector( halfhull, halfhull, GetHullHeight() - height );
    Assert( mins.z < maxs.z );


    Vector vecStepDown;
    if ( m_bDoStepDownTrace )
    {
        vecStepDown = Vector( 0, 0, -height - 0.01f );
    }
    else
    {
        vecStepDown = Vector( 0, 0, -0.01f );
    }


    Vector offGround = Vector( 0.0f, 0.0f, height );


    CMoveFilter filter( GetNPC(), COLLISION_GROUP_NPC );
    trace_t tr;
    UTIL_TraceHull(
        startPos + offGround,
        startPos + vecStepDown,
        mins, maxs,
        MASK_NPCSOLID, &filter, &tr );

    if ( npcr_debug_navigator_ground.GetInt() > 0 )
    {
        bool bInvalid = tr.startsolid;
        switch ( npcr_debug_navigator_ground.GetInt() )
        {
        case 1 :
            NDebugOverlay::Box( startPos + offGround, mins, maxs, bInvalid ? 255 : 0, (!bInvalid) ? 255 : 0, 0, 0, 0.1f );
            break;
        case 2 :
            NDebugOverlay::Box( tr.endpos, mins, maxs, bInvalid ? 255 : 0, (!bInvalid) ? 255 : 0, 0, 0, 0.1f );
            break;
        default :
            break;
        }
    }


    bool bValidPos = true;

    // We hit something!
    if ( tr.fraction < 1.0f && !tr.startsolid )
    {
        bool bCanStand = CanStandOnNormal( tr.plane.normal );


        // NEVER get on a poor normal if we're on good ground.
        //
        // This fixes:
        //
        // Snapping when the npc is hugging something,
        // with an absurd normal z, like 0.01. (essentially a straight wall, how did we hit it???)
        //
        // Npcs getting stuck on certain slanted surfaces (mostly props)
        if ( bCanStand || !IsOnGround() )
        {
            GetNPC()->SetPosition( tr.endpos );
        }
        else
        {
            // Nope, go back to a valid pos.
            bValidPos = false;
        }


        if ( bCanStand )
        {
            // This ground z offset is used to fix npcs getting stuck to a ceiling.
            if ( tr.endpos.z > startPos.z )
            {
                m_flGroundZOffset = tr.endpos.z - startPos.z;
            }
            else m_flGroundZOffset = 0.0f;


            m_vecGroundNormal = tr.plane.normal;

            // We're done being in the air, step trace again.
            m_bDoStepDownTrace = true;

            return tr.m_pEnt;
        }
    }

    // This is not a valid position, go back to valid one.
    if ( !bValidPos )
    {
        GetNPC()->SetPosition( m_vecLastValidPos );

        // We have to force gravity here or we'll be floating in the ceiling being stuck
        m_bForceGravity = true;

        return tr.m_pEnt;
    }


    // We're off the ground, wait until we hit the ground properly.
    m_bDoStepDownTrace = false;
    m_vecGroundNormal.z = 0.0f;

    return nullptr;
}

void NPCR::CBaseMotor::Update()
{
    m_bDoMove = false;
    m_vecDesiredMoveDir = vec3_origin;
}

void NPCR::CBaseMotor::OnLandedGround( CBaseEntity* pGround )
{
    m_hGroundEnt.Set( pGround );
    m_bOnGround = true;
}

void NPCR::CBaseMotor::OnLeftGround( CBaseEntity* pOldGround )
{
    m_hGroundEnt.Set( nullptr );
    m_bOnGround = false;
}

void NPCR::CBaseMotor::Jump()
{
    GetNPC()->OnLeftGround( nullptr );
    m_bDoStepDownTrace = false;

    Vector movedir = GetVelocity();
    movedir.z = 0.0f;
    m_vecMoveDir = movedir.Normalized();

    // Make sure we don't apply ground move.
    m_vecGroundNormal.z = 0.0f;
}

void NPCR::CBaseMotor::NavJump( const Vector& vecGoal, float flOverrideHeight )
{
    GetNPC()->OnLeftGround( nullptr );
    m_bDoStepDownTrace = false;
    

    // Step up off the ground so we don't snap back to ground.
    Vector pos = GetNPC()->GetPosition();


    float halfhull = GetHullWidth() / 2.0f;
    Vector mins = Vector( -halfhull, -halfhull, 0.0f );
    Vector maxs = Vector( halfhull, halfhull, 2.0f );

    CMoveFilter filter( GetNPC(), COLLISION_GROUP_NPC );
    trace_t tr;
    UTIL_TraceHull(
        pos + Vector( 0, 0, 32.0f ),
        pos - Vector( 0, 0, -1.0f ),
        mins, maxs,
        MASK_NPCSOLID, &filter, &tr );

    Vector normal( 0.0f, 0.0f, 1.0f );
    if ( !tr.startsolid && tr.fraction < 1.0f )
    {
        pos = tr.endpos;

        if ( tr.plane.normal.z > 0.0f )
            normal = tr.plane.normal;
    }

    pos += normal * npcr_motor_jump_groundoffset.GetFloat();
    GetNPC()->SetPosition( pos );

    float minheight = vecGoal.z - pos.z + GetHullHeight();
    if ( flOverrideHeight > 0.0f )
        minheight = flOverrideHeight;

    minheight = MAX( 8.0f, minheight );

    Vector apex;
    Vector vel = CalcJumpLaunchVelocity( pos, vecGoal, GetGravity(), &minheight, 350.0f, &apex );

    SetVelocity( vel );


    // Make sure we don't apply ground move.
    m_vecGroundNormal.z = 0.0f;

    GetNPC()->OnNavJump();
}

// Shamelessly copied from ai code.
Vector NPCR::CBaseMotor::CalcJumpLaunchVelocity( const Vector& startPos, const Vector& endPos, float flGravity, float* pminHeight, float maxHorzVelocity, Vector* pvecApex )
{
    // Get the height I have to jump to get to the target
    float	stepHeight = endPos.z - startPos.z;

    // get horizontal distance to target
    Vector targetDir2D	= endPos - startPos;
    targetDir2D.z = 0;
    float distance = VectorNormalize(targetDir2D);

    Assert( maxHorzVelocity > 0 );

    // get minimum times and heights to meet ideal horz velocity
    float minHorzTime = distance / maxHorzVelocity;
    float minHorzHeight = 0.5 * flGravity * (minHorzTime * 0.5) * (minHorzTime * 0.5);

    // jump height must be enough to hang in the air
    *pminHeight = MAX( *pminHeight, minHorzHeight );
    // jump height must be enough to cover the step up
    *pminHeight = MAX( *pminHeight, stepHeight );

    // time from start to apex
    float t0 = sqrt( ( 2.0 * *pminHeight) / flGravity );
    // time from apex to end
    float t1 = sqrt( ( 2.0 * fabs( *pminHeight - stepHeight) ) / flGravity );

    float velHorz = distance / (t0 + t1);

    Vector jumpVel = targetDir2D * velHorz;

    jumpVel.z = (float)sqrt(2.0f * flGravity * (*pminHeight));

    if (pvecApex)
    {
        *pvecApex = startPos + targetDir2D * velHorz * t0 + Vector( 0, 0, *pminHeight );
    }

    // -----------------------------------------------------------
    // Make the horizontal jump vector and add vertical component
    // -----------------------------------------------------------

    return jumpVel;
}

void NPCR::CBaseMotor::Climb()
{

}


void NPCR::CBaseMotor::MoveTowards( const Vector& vecNewPos )
{
    const Vector newpos = HandleCollisions( vecNewPos );

    GetOuter()->SetAbsOrigin( newpos );
}

Vector NPCR::CBaseMotor::HandleCollisions( const Vector& vecGoal )
{
    VPROF_BUDGET( "CBaseMotor::HandleCollisions", "NPCR" );


    m_bAdjustVel = false;


    Vector validPos = GetNPC()->GetPosition();
    
    float halfhull = GetHullWidth() / 2.0f;

    // If we're on ground, ignore step height.
    // This is important, because it is very easy to get stuck on slopes, etc.
    Vector mins( -halfhull, -halfhull, IsOnGround() ? GetStepHeight() : 0.0f );
    Vector maxs( halfhull, halfhull, GetHullHeight() - m_flGroundZOffset );

    // Should never happen.
    Assert( mins.z < maxs.z );


    int limit = 3;
    Vector startPos = validPos;
    Vector goalPos = vecGoal;

    if ( npcr_debug_navigator.GetBool() )
    {
        NDebugOverlay::Box( startPos, mins, maxs, 0, 255, 0, 0, 0.1f );
    }


    trace_t tr;
    while ( limit-- > 0 )
    {
        CMoveFilter filter( GetNPC(), COLLISION_GROUP_NPC );
        UTIL_TraceHull( startPos, goalPos, mins, maxs, MASK_NPCSOLID, &filter, &tr );

        if ( !tr.DidHit() )
        {
            validPos = goalPos;
            break;
        }

        if ( tr.m_pEnt )
        {
            GetNPC()->OnTouch( tr.m_pEnt, &tr );
        }

        // We presumably hit ground, start tracing for steps again.
        // NOTE: Fixes not having a ground ent on moving lifts
        if ( CanStandOnNormal( tr.plane.normal ) )
            m_bDoStepDownTrace = true;

        m_bAdjustVel = true;

        if ( !tr.startsolid && goalPos.DistToSqr( tr.endpos ) < 1.0f )
        {
            validPos = tr.endpos;
            break;
        }

        if ( tr.startsolid )
        {
            if ( npcr_debug_navigator.GetBool() )
            {
                NDebugOverlay::Box( startPos, mins, maxs, 255, 0, 0, 0, 1.0f );
            }

            // HACK: We are constantly getting stuck on physics objects.
            CBaseEntity* pEnt = tr.m_pEnt;
            if ( pEnt && pEnt->VPhysicsGetObject() )
            {
                IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

                Vector vel;
                pPhys->GetVelocity( &vel, nullptr );
                if ( pPhys->IsMotionEnabled() && vel.LengthSqr() < (10.0f*10.0f) )
                {
                    Vector dir = pEnt->WorldSpaceCenter() - startPos;
                    dir.NormalizeInPlace();
                    dir *= 100.0f;

                    pPhys->AddVelocity( &dir, nullptr );

                    if ( npcr_debug_navigator.GetBool() )
                    {
                        NDebugOverlay::HorzArrow( startPos, pEnt->WorldSpaceCenter(), 12.0f, 255, 0, 0, 255, true, 1.0f );
                    }
                }
            }

            if ( m_bHasValidPos )
            {
                Assert( validPos != m_vecLastValidPos );
                validPos = m_vecLastValidPos;
            }

            m_bAdjustVel = false;
            break;
        }


        Vector fullMove = goalPos - startPos;
        Vector leftToMove = fullMove * ( 1.0f - tr.fraction );


        // Don't bother going down when we're on ground and there's a slanted wall.
        // This stops npcs getting stuck in the ground.
        if (!CanStandOnNormal( tr.plane.normal )
        &&  IsOnGround() )
        //&&  fullMove.z > 0.0f )
        {
            fullMove.z = 0.0f;
            tr.plane.normal.z = 0.0f;
            tr.plane.normal.NormalizeInPlace();
        }

        float blocked = DotProduct( tr.plane.normal, leftToMove );
        Vector unconstrained = fullMove - blocked * tr.plane.normal;

        Vector remainingMove = startPos + unconstrained;
        if ( remainingMove.DistToSqr( tr.endpos ) < 1.0f )
        {
            validPos = tr.endpos;
            break;
        }


        goalPos = remainingMove;
    }

    if ( !tr.startsolid )
    {
        m_vecLastValidPos = validPos;
        m_bHasValidPos = true;
    }
    
    return validPos;
}

void NPCR::CBaseMotor::Approach( const Vector& vecDesiredGoal )
{
    m_vecDesiredMoveDir += ( vecDesiredGoal - GetNPC()->GetPosition() );

    m_bDoMove = true;
}

void NPCR::CBaseMotor::FaceTowards( const Vector& vecPos )
{
    QAngle ang;
    VectorAngles( vecPos - GetOuter()->EyePosition(), ang );
    ang.x = AngleNormalize( ang.x );
    ang.y = AngleNormalize( ang.y );

    FaceTowards( ang );
}

void NPCR::CBaseMotor::FaceTowards( const QAngle& angGoal )
{
    QAngle curAngles = GetNPC()->GetEyeAngles();
    float time = GetNPC()->GetUpdateInterval();
    
    float dx = UTIL_AngleDiff( angGoal.x, curAngles.x );
    float dy = UTIL_AngleDiff( angGoal.y, curAngles.y );

    float mx = GetPitchRate( dx ) * time;
    float my = GetYawRate( dy ) * time;
    
    if ( UsePitch() )
    {
        if ( dx < -mx )
            curAngles.x -= mx;
        else if ( dx > mx )
            curAngles.x += mx;
        else
            curAngles.x = angGoal.x;
    }


    if ( dy < -my )
        curAngles.y -= my;
    else if ( dy > my )
        curAngles.y += my;
    else
        curAngles.y = angGoal.y;
    

    GetNPC()->SetEyeAngles( curAngles );
}

void NPCR::CBaseMotor::FaceTowards( float yaw )
{
    QAngle ang = GetNPC()->GetEyeAngles();
    float time = GetNPC()->GetUpdateInterval();
    
    float dy = UTIL_AngleDiff( yaw, ang.y );

    float my = GetYawRate( dy ) * time;

    if ( dy < -my )
        ang.y -= my;
    else if ( dy > my )
        ang.y += my;
    else
        ang.y = yaw;
    

    GetNPC()->SetEyeAngles( ang );
}

bool NPCR::CBaseMotor::IsFacing( const Vector& vecPos, float grace ) const
{
    QAngle angGoal;
    VectorAngles( vecPos - GetOuter()->EyePosition(), angGoal );
    angGoal.x = AngleNormalize( angGoal.x );
    angGoal.y = AngleNormalize( angGoal.y );

    return IsFacing( angGoal, grace );
}

bool NPCR::CBaseMotor::IsFacing( const QAngle& angGoal, float grace ) const
{
    grace = abs( grace );

    QAngle curAngles = GetNPC()->GetEyeAngles();

    float dx = UsePitch() ? UTIL_AngleDiff( angGoal.x, curAngles.x ) : 0.0f;
    float dy = UTIL_AngleDiff( angGoal.y, curAngles.y );

    return abs( dx ) <= grace && abs( dy ) <= grace;
}
