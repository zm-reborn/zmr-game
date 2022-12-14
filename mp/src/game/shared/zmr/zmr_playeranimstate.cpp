#include "cbase.h"
#include "base_playeranimstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "datacache/imdlcache.h"


#include "zmr_playeranimstate.h"

#include "zmr_player_shared.h"
#include "zmr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MAX_FEET_GOAL_DELTA         45.0f

// Don't let the head spass out.
#define ZM_LOOKAT_UPDATE_TIME       0.1f
#define ZM_LOOKAT_DIST              100.0f
#define ZM_LOOKAT_DOT               0.5f // < 0 is way too big of an angle.


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CZMPlayerAnimState* CreateZMPlayerAnimState( CZMPlayer* pPlayer )
{
    MDLCACHE_CRITICAL_SECTION();

    // Setup the movement data.
    MultiPlayerMovementData_t movementData;
    movementData.m_flBodyYawRate = 720.0f;
    movementData.m_flRunSpeed = ZM_WALK_SPEED;
    movementData.m_flWalkSpeed = ZM_WALK_SPEED;
    movementData.m_flSprintSpeed = -1.0f;

    // Create animation state for this player.
    auto* pRet = new CZMPlayerAnimState( pPlayer, movementData );

    // Specific ZM player initialization.
    pRet->InitZMAnimState( pPlayer );

    return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CZMPlayerAnimState::CZMPlayerAnimState( CBasePlayer* pPlayer, MultiPlayerMovementData_t& movementData )
    : CMultiPlayerAnimState( pPlayer, movementData )
{
    m_pZMPlayer = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CZMPlayerAnimState::~CZMPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize ZM specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::InitZMAnimState( CZMPlayer* pPlayer )
{
    Assert( pPlayer );
    m_pZMPlayer = pPlayer;

    m_blinkTimer.Invalidate();
    m_flLastLookAtUpdate = 0.0f;

    m_flLastBodyYaw = 0.0f;
    m_flCurrentHeadYaw = 0.0f;
    m_flCurrentHeadPitch = 0.0f;
}

// For animations, why would ever want to use the absolute velocity?
// You want the local velocity, the thing that's not affected by outside sources (ie. moving platforms)
// ???????
void CZMPlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
    vel = GetZMPlayer()->GetLocalVelocity();
}

bool CZMPlayerAnimState::ShouldUpdateAnimState()
{
    // Being ZM != EF_NODRAW.
    if ( m_pZMPlayer->IsEffectActive( EF_NODRAW ) )
        return false;

    if ( m_pZMPlayer->IsZM() || m_pZMPlayer->IsObserver() )
        return false;

    // Why would we update anims if we're not getting updates from server
#ifdef CLIENT_DLL
    if ( m_pZMPlayer->IsDormant() )
        return false;
#endif

    return m_pZMPlayer->IsAlive() || m_bDying;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::ClearAnimationState()
{
    BaseClass::ClearAnimationState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CZMPlayerAnimState::TranslateActivity( Activity actDesired )
{
    // Hook into baseclass when / if ZM player models get swim animations.
    Activity translateActivity = actDesired; //BaseClass::TranslateActivity( actDesired );

    if ( m_pZMPlayer && m_pZMPlayer->GetActiveWeapon() )
    {
        translateActivity = m_pZMPlayer->GetActiveWeapon()->ActivityOverride( translateActivity, false );
    }

    return translateActivity;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
    // Profile the animation update.
    VPROF( "CZMPlayerAnimState::Update" );

    // Get the studio header for the player.
    auto* pStudioHdr = GetZMPlayer()->GetModelPtr();
    if ( !pStudioHdr )
        return;


    // We're not updating if we don't need to.
#ifdef CLIENT_DLL
    // This will screw up localplayer's ragdoll.
    //if ( pZMPlayer->IsLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer() )
    //    return;
#endif

    // Check to see if we should be updating the animation state - dead, ragdolled?
    if ( !ShouldUpdateAnimState() )
    {
        ClearAnimationState();
        return;
    }

    // Store the eye angles.
    m_flEyeYaw = AngleNormalize( eyeYaw );
    m_flEyePitch = AngleNormalize( eyePitch );

    // Compute the player sequences.
    ComputeSequences( pStudioHdr );

    if ( SetupPoseParameters( pStudioHdr ) )
    {
        // Pose parameter - what direction are the player's legs running in.
        ComputePoseParam_MoveYaw( pStudioHdr );

        // Pose parameter - Torso aiming (up/down).
        ComputePoseParam_AimPitch( pStudioHdr );

        // Pose parameter - Torso aiming (rotation).
        ComputePoseParam_AimYaw( pStudioHdr );

        // ZMR
        // Pose parameter - Head + eyes
        // NOTE: If we ever decide to care/care less about lag compensation on players, we can just make these shared/all on client.
        // IIRC pose parameters aren't lag compensated anyway.
#ifdef CLIENT_DLL
        ComputePoseParam_Head( pStudioHdr );
#endif
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData )
{
    Activity iGestureActivity = ACT_INVALID;

    switch( playerAnim )
    {
    case PLAYERANIMEVENT_ATTACK_PRIMARY:
        {
            // Weapon primary fire.
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
            else
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );

            iGestureActivity = ACT_VM_PRIMARYATTACK;
            break;
        }

    case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
        {
            if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );

            break;
        }
    case PLAYERANIMEVENT_ATTACK_SECONDARY:
        {
            // Weapon secondary fire.
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
            else
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );

            iGestureActivity = ACT_VM_PRIMARYATTACK;
            break;
        }
    case PLAYERANIMEVENT_ATTACK_PRE:
        {
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING ) 
            {
                // Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
                iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
            }
            else
            {
                // Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
                iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
            }

            RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false );

            break;
        }
    case PLAYERANIMEVENT_ATTACK_POST:
        {
            RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE );
            break;
        }

    case PLAYERANIMEVENT_RELOAD:
        {
            // Weapon reload.
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
            else
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
            break;
        }
    case PLAYERANIMEVENT_RELOAD_LOOP:
        {
            // Weapon reload.
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
            else
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
            break;
        }
    case PLAYERANIMEVENT_RELOAD_END:
        {
            // Weapon reload.
            if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
            else
                RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
            break;
        }
    default:
        {
            BaseClass::DoAnimationEvent( playerAnim, nData );
            break;
        }
    }

#ifdef CLIENT_DLL
    // ZMRCHANGE: This only gets in the way. We send weapon animations ourselves.
    /*
    // Make the weapon play the animation as well
    if ( iGestureActivity != ACT_INVALID )
    {
        CBaseCombatWeapon *pWeapon = m_pZMPlayer->GetActiveWeapon();
        if ( pWeapon )
        {
//			pWeapon->EnsureCorrectRenderingModel();
            pWeapon->SendWeaponAnim( iGestureActivity );
//			// Force animation events!
//			pWeapon->ResetEventsParity();		// reset event parity so the animation events will occur on the weapon. 
            pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
        }
    }
    */
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CZMPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
    //bool bInWater = BaseClass::HandleSwimming( idealActivity );
    //
    //return bInWater;

    // ZMRCHANGE: We don't have any swimming animations.
    if ( m_pZMPlayer->GetWaterLevel() >= WL_Waist )
    {
        idealActivity = ACT_MP_JUMP;
        return true; 
    }
    
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMPlayerAnimState::HandleMoving( Activity &idealActivity )
{
    float flSpeed = GetOuterXYSpeed();

    if ( flSpeed <= MOVING_MINIMUM_SPEED )
    {
        return false;
    }


    if ( m_pZMPlayer->IsRat() )
    {
        idealActivity = flSpeed > 100.0f ? ACT_MP_RUN : ACT_MP_WALK;
    }
    else
    {
        idealActivity = ACT_MP_RUN;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMPlayerAnimState::HandleDucking( Activity &idealActivity )
{
    if ( m_pZMPlayer->IsRat() )
    {
        return false;
    }

    if ( m_pZMPlayer->GetFlags() & FL_DUCKING )
    {
        if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED )
        {
            idealActivity = ACT_MP_CROUCH_IDLE;		
        }
        else
        {
            idealActivity = ACT_MP_CROUCHWALK;		
        }

        return true;
    }
    
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
bool CZMPlayerAnimState::HandleJumping( Activity &idealActivity )
{
    Vector vecVelocity;
    GetOuterAbsVelocity( vecVelocity );

    if ( m_bJumping )
    {
        static bool bNewJump = false; //Tony; hl2mp players only have a 'hop'

        if ( m_bFirstJumpFrame )
        {
            m_bFirstJumpFrame = false;
            RestartMainSequence();	// Reset the animation.
        }

        // Reset if we hit water and start swimming.
        if ( m_pZMPlayer->GetWaterLevel() >= WL_Waist )
        {
            m_bJumping = false;
            RestartMainSequence();
        }
        // Don't check if he's on the ground for a sec.. sometimes the client still has the
        // on-ground flag set right when the message comes in.
        else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
        {
            if ( m_pZMPlayer->GetFlags() & FL_ONGROUND )
            {
                m_bJumping = false;
                RestartMainSequence();

                if ( bNewJump )
                {
                    RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
                }
            }
        }

        // if we're still jumping
        if ( m_bJumping )
        {
            if ( bNewJump )
            {
                if ( (gpGlobals->curtime - m_flJumpStartTime) > 0.5f )
                {
                    idealActivity = ACT_MP_JUMP_FLOAT;
                }
                else
                {
                    idealActivity = ACT_MP_JUMP_START;
                }
            }
            else
            {
                idealActivity = ACT_MP_JUMP;
            }
        }
    }	

    if ( m_bJumping )
        return true;

    return false;
}

bool CZMPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
    // Check to see if this has already been done.
    if ( m_bPoseParameterInit )
        return true;

    // Save off the pose parameter indices.
    if ( !pStudioHdr )
        return false;


    
    m_bIs9Way = false;

    m_PoseParameterData.m_iMoveX = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "move_x" );
    m_PoseParameterData.m_iMoveY = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "move_y" );

    if ( m_PoseParameterData.m_iMoveX >= 0 && m_PoseParameterData.m_iMoveY >= 0 )
    {
        m_bIs9Way = true;

        m_PoseParameterData.m_iAimPitch = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "body_pitch" );
        m_PoseParameterData.m_iAimYaw = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "body_yaw" );
    }
    //
    // Fallback to 8way blending
    //
    else
    {
        // 8way blending doesn't use move_x/move_y
        // and it only has one "move_yaw" param.
        m_PoseParameterData.m_iMoveX =
        m_PoseParameterData.m_iMoveY = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "move_yaw" );
        if ( ( m_PoseParameterData.m_iMoveX < 0 ) || ( m_PoseParameterData.m_iMoveY < 0 ) )
            return false;

        m_PoseParameterData.m_iAimPitch = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "aim_pitch" );
        if ( m_PoseParameterData.m_iAimPitch < 0 )
            return false;

        m_PoseParameterData.m_iAimYaw = m_pZMPlayer->LookupPoseParameter( pStudioHdr, "aim_yaw" );
        if ( m_PoseParameterData.m_iAimYaw < 0 )
            return false;
    }


#ifdef CLIENT_DLL
    // ZMR
    m_headYawPoseParam = m_pZMPlayer->LookupPoseParameter( "head_yaw" );
    if ( m_headYawPoseParam < 0 )
        return false;
    m_pZMPlayer->GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

    m_headPitchPoseParam = m_pZMPlayer->LookupPoseParameter( "head_pitch" );
    if ( m_headPitchPoseParam < 0 )
        return false;
    m_pZMPlayer->GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );
#endif

    m_bPoseParameterInit = true;

    return true;
}
float SnapYawTo( float flValue );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::EstimateYaw()
{
    // Get the frame time.
    float flDeltaTime = gpGlobals->frametime;
    if ( flDeltaTime == 0.0f )
        return;

    if ( Uses9WayAnim() )
    {
        // Get the player's velocity and angles.
        Vector vecEstVelocity;
        GetOuterAbsVelocity( vecEstVelocity );
        QAngle angles = GetBasePlayer()->GetLocalAngles();

        // If we are not moving, sync up the feet and eyes slowly.
        if ( vecEstVelocity.x == 0.0f && vecEstVelocity.y == 0.0f )
        {
            float flYawDelta = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
            flYawDelta = AngleNormalize( flYawDelta );

            if ( flDeltaTime < 0.25f )
            {
                flYawDelta *= ( flDeltaTime * 4.0f );
            }
            else
            {
                flYawDelta *= flDeltaTime;
            }

            m_PoseParameterData.m_flEstimateYaw += flYawDelta;
            AngleNormalize( m_PoseParameterData.m_flEstimateYaw );
        }
        else
        {
            m_PoseParameterData.m_flEstimateYaw = ( atan2( vecEstVelocity.y, vecEstVelocity.x ) * 180.0f / M_PI );
            m_PoseParameterData.m_flEstimateYaw = clamp( m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f );
        }
    }
    else
    {
        // Get the player's velocity and angles.
        Vector vecEstVelocity;
        GetOuterAbsVelocity( vecEstVelocity );
        // ZMRCHANGE: Use eye yaw.
        //QAngle angles = GetBasePlayer()->GetLocalAngles();
        QAngle angles = QAngle( 0.0f, m_flEyeYaw, 0.0f );

        if ( vecEstVelocity.y == 0 && vecEstVelocity.x == 0 )
        {
            float flYawDiff = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
            flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
            if (flYawDiff > 180)
                flYawDiff -= 360;
            if (flYawDiff < -180)
                flYawDiff += 360;

            if (flDeltaTime < 0.25)
                flYawDiff *= flDeltaTime * 4;
            else
                flYawDiff *= flDeltaTime;

            m_PoseParameterData.m_flEstimateYaw += flYawDiff;
            m_PoseParameterData.m_flEstimateYaw = m_PoseParameterData.m_flEstimateYaw - (int)(m_PoseParameterData.m_flEstimateYaw / 360) * 360;
        }
        else
        {
            m_PoseParameterData.m_flEstimateYaw = (atan2(vecEstVelocity.y, vecEstVelocity.x) * 180 / M_PI);

            if (m_PoseParameterData.m_flEstimateYaw > 180)
                m_PoseParameterData.m_flEstimateYaw = 180;
            else if (m_PoseParameterData.m_flEstimateYaw < -180)
                m_PoseParameterData.m_flEstimateYaw = -180;
        }
    }
}
//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
float SnapYawTo( float flValue );
void CZMPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
    // Get the estimated movement yaw.
    EstimateYaw();

    if ( Uses9WayAnim() )
    {
        ConVarRef mp_slammoveyaw("mp_slammoveyaw");

        // Get the view yaw.
        float flAngle = AngleNormalize( m_flEyeYaw );

        // Calc side to side turning - the view vs. movement yaw.
        float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
        flYaw = AngleNormalize( -flYaw );

        // Get the current speed the character is running.
        bool bIsMoving;
        float flPlaybackRate = 	CalcMovementPlaybackRate( &bIsMoving );

        // Setup the 9-way blend parameters based on our speed and direction.
        Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
        if ( bIsMoving )
        {
            if ( mp_slammoveyaw.GetBool() )
                flYaw = SnapYawTo( flYaw );

            vecCurrentMoveYaw.x = cosf( DEG2RAD( flYaw ) ) * flPlaybackRate;
            vecCurrentMoveYaw.y = sinf( DEG2RAD( flYaw ) ) * flPlaybackRate;
        }

        GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
        GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );

        m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
    }
    else
    {
        // view direction relative to movement
        float flYaw;	 

        // ZMRCHANGE: This fucked feet movement.
        // I spent way too much time (AGAIN) debugging this shit.
        // The above code has it right. Why the fuck was it changed?
        // AFAIK, it wouldn't only affect us. Really weird that it wasn't caught. It probably was but I copied an older version. Oh well...
        //QAngle	angles = GetBasePlayer()->GetAbsAngles();
        //float ang = angles[ YAW ];
        float ang = m_flEyeYaw;
        if ( ang > 180.0f )
        {
            ang -= 360.0f;
        }
        else if ( ang < -180.0f )
        {
            ang += 360.0f;
        }

        // calc side to side turning
        flYaw = ang - m_PoseParameterData.m_flEstimateYaw;
        // Invert for mapping into 8way blend
        flYaw = -flYaw;
        flYaw = flYaw - (int)(flYaw / 360) * 360;

        if (flYaw < -180)
        {
            flYaw = flYaw + 360;
        }
        else if (flYaw > 180)
        {
            flYaw = flYaw - 360;
        }

        //Tony; oops, i inverted this previously above.
        m_pZMPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, flYaw );
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
    // Get the view pitch.
    float flAimPitch = m_flEyePitch;

    // Set the aim pitch pose parameter and save.
    if ( m_PoseParameterData.m_iAimPitch >= 0 )
    {
        m_pZMPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch );
    }
    m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
    // Get the movement velocity.
    Vector vecVelocity;
    GetOuterAbsVelocity( vecVelocity );

    // Check to see if we are moving.
    bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

    // If we are moving or are prone and undeployed.
    if ( bMoving || m_bForceAimYaw )
    {
        // The feet match the eye direction when moving - the move yaw takes care of the rest.
        m_flGoalFeetYaw = m_flEyeYaw;
    }
    // Else if we are not moving.
    else
    {
        // Initialize the feet.
        if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
        {
            m_flGoalFeetYaw	= m_flEyeYaw;
            m_flCurrentFeetYaw = m_flEyeYaw;
            m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
        }
        // Make sure the feet yaw isn't too far out of sync with the eye yaw.
        // TODO: Do something better here!
        else
        {
            float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

            const float flMaxDelta = MAX_FEET_GOAL_DELTA;

            if ( fabs( flYawDelta ) > flMaxDelta )
            {
                m_flGoalFeetYaw = m_flEyeYaw;
            }
        }
    }

    // Fix up the feet yaw.
    m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
    if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
    {
        if ( m_bForceAimYaw )
        {
            m_flCurrentFeetYaw = m_flGoalFeetYaw;
        }
        else
        {
            ConvergeYawAngles( m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
            m_flLastAimTurnTime = gpGlobals->curtime;
        }
    }

    // Rotate the body into position.
    m_angRender[YAW] = m_flCurrentFeetYaw;

    // Find the aim(torso) yaw base on the eye and feet yaws.
    float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
    flAimYaw = AngleNormalize( flAimYaw );

    // Set the aim yaw and save.
    if ( m_PoseParameterData.m_iAimYaw >= 0 )
    {
        m_pZMPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw );
        m_flCurrentAimYaw = flAimYaw;
    }
    else
    {
        m_flCurrentAimYaw = 0.0f;
    }

    m_DebugAnimData.m_flAimYaw	= flAimYaw;

    // Turn off a force aim yaw - either we have already updated or we don't need to.
    m_bForceAimYaw = false;

#ifndef CLIENT_DLL
    QAngle angle = m_pZMPlayer->GetAbsAngles();
    angle[YAW] = m_flCurrentFeetYaw;

    m_pZMPlayer->SetAbsAngles( angle );
#endif
}

#ifdef CLIENT_DLL
void CZMPlayerAnimState::UpdateLookAt()
{
    if ( (gpGlobals->curtime - m_flLastLookAtUpdate) < ZM_LOOKAT_UPDATE_TIME )
        return;

    if ( !m_pZMPlayer->IsAlive() )
        return;


    // Update player model's look at target.
    bool bFoundViewTarget = false;
    
    Vector vForward;
    AngleVectors( m_pZMPlayer->EyeAngles(), &vForward );

    Vector vMyOrigin = m_pZMPlayer->GetAbsOrigin();

    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        C_ZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );
        if( !pPlayer ) continue;

        // Don't look at dead players :(
        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsZM() || pPlayer->IsObserver() )
            continue;

        if ( pPlayer->IsEffectActive( EF_NODRAW ) )
            continue;

        if ( pPlayer->entindex() == m_pZMPlayer->entindex() )
            continue;


        Vector vTargetOrigin = pPlayer->GetAbsOrigin();

        Vector vDir = vTargetOrigin - vMyOrigin;
        
        if ( vDir.LengthSqr() > (ZM_LOOKAT_DIST*ZM_LOOKAT_DIST) ) 
            continue;

        VectorNormalize( vDir );

        if ( DotProduct( vForward, vDir ) < ZM_LOOKAT_DOT )
                continue;

        m_vLookAtTarget = pPlayer->EyePosition();
        bFoundViewTarget = true;
        break;
    }

    if ( !bFoundViewTarget )
    {
        m_vLookAtTarget = m_pZMPlayer->EyePosition() + vForward * 512.0f;
    }


    m_flLastLookAtUpdate = gpGlobals->curtime;
}

void CZMPlayerAnimState::ComputePoseParam_Head( CStudioHdr* hdr )
{
    UpdateLookAt();


    // orient eyes
    m_pZMPlayer->SetLookat( m_vLookAtTarget );

    // blinking
    if ( m_blinkTimer.IsElapsed() )
    {
        m_pZMPlayer->BlinkEyes();
        m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
    }

    // Figure out where we want to look in world space.
    QAngle desiredAngles;
    Vector to = m_vLookAtTarget - m_pZMPlayer->EyePosition();
    VectorAngles( to, desiredAngles );

    // Figure out where our body is facing in world space.
    QAngle bodyAngles( 0, 0, 0 );
    bodyAngles[YAW] = m_flGoalFeetYaw; // Just use our feet goal yaw.


    float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
    m_flLastBodyYaw = bodyAngles[YAW];
    

    // Set the head's yaw.
    // Take into account aim yaw.
    float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] - m_flCurrentAimYaw );
    desired = clamp( desired, m_headYawMin, m_headYawMax );
    m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

    // Counterrotate the head from the body rotation so it doesn't rotate past its target.
    m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );
    desired = clamp( desired, m_headYawMin, m_headYawMax );
    
    m_pZMPlayer->SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

    
    // Set the head's pitch.
    desired = AngleNormalize( desiredAngles[PITCH] );
    desired = clamp( desired, m_headPitchMin, m_headPitchMax );
    
    m_flCurrentHeadPitch = ApproachAngle( desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
    m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
    m_pZMPlayer->SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Override the default, because hl2mp models don't use moveX
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CZMPlayerAnimState::GetCurrentMaxGroundSpeed()
{
    CStudioHdr *pStudioHdr = m_pZMPlayer->GetModelPtr();

    if ( pStudioHdr == NULL )
        return 1.0f;

//	float prevX = GetBasePlayer()->GetPoseParameter( m_PoseParameterData.m_iMoveX );
    float prevY = m_pZMPlayer->GetPoseParameter( m_PoseParameterData.m_iMoveY );

    float d = sqrtf( /*prevX * prevX + */prevY * prevY );
    float newY;//, newX;
    if ( d == 0.0 )
    { 
//		newX = 1.0;
        newY = 0.0;
    }
    else
    {
//		newX = prevX / d;
        newY = prevY / d;
    }

//	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, newX );
    m_pZMPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, newY );

    float speed = m_pZMPlayer->GetSequenceGroundSpeed( m_pZMPlayer->GetSequence() );

//	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, prevX );
    m_pZMPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, prevY );

    return speed;
}
