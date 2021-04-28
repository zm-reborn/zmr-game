#include "cbase.h"


#include "zmr_player_shared.h"
#include "zmr_viewmodel.h"

#ifdef CLIENT_DLL
#include <materialsystem/imaterialvar.h>
#include "proxyentity.h"
#include "ivieweffects.h"
#include "prediction.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
ConVar zm_cl_crosshair_draw_ironsights( "zm_cl_crosshair_draw_ironsights", "0", FCVAR_ARCHIVE, "Is the crosshair drawn when zoomed in into ironsights." );
#endif // CLIENT_DLL


static float ApproachSmooth( float target, float value, float speed, float smoothness_start, float epsilon = 0.01f );


LINK_ENTITY_TO_CLASS( zm_viewmodel, CZMViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( ZMViewModel, DT_ZM_ViewModel )

BEGIN_NETWORK_TABLE( CZMViewModel, DT_ZM_ViewModel )
#ifdef CLIENT_DLL
    RecvPropFloat( RECVINFO( m_flClr[0] ) ),
    RecvPropFloat( RECVINFO( m_flClr[1] ) ),
    RecvPropFloat( RECVINFO( m_flClr[2] ) ),
#else
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 0 ), -1, 0, 0.0f, 1.0f ),
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 1 ), -1, 0, 0.0f, 1.0f ),
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 2 ), -1, 0, 0.0f, 1.0f ),


    SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
    SendPropExclude( "DT_BaseViewModel", "m_flPoseParameter" ),

    SendPropExclude( "DT_BaseAnimating", "m_flEncodedController" ),

    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_ZMViewModel )

    DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_ARRAY( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

END_PREDICTION_DATA()
#endif

#ifdef CLIENT_DLL
CZMViewModel::CZMViewModel() : m_LagAnglesHistory( "CZMViewModel::m_LagAnglesHistory" )
#else
CZMViewModel::CZMViewModel()
#endif
{
#ifdef CLIENT_DLL
    m_bDrawVM = true;
    m_iOverrideModelIndex = -1;
    m_pOverrideModelWeapon = nullptr;
    m_pLastWeapon = nullptr;

    m_bInIronSight = false;
    m_flIronSightFrac = 0.0f;


    m_vLagAngles.Init();
    AddVar( &m_vLagAngles, &m_LagAnglesHistory, 0, true );

    m_flLastImpactValue = 0.0f;
    m_flLastImpactGroundOffsetZ = 0;
    m_bWasOnGround = false;

    m_iPoseParamMoveX = -1;
    m_iPoseParamVertAim = -1;
    m_iAttachmentIronsight = -1;
    m_iAttachmentScopeEnd = -1;

    m_flLastMoveX = 0.0f;

    m_vecLastVel.Init();
#else
    SetModelColor2( 1.0f, 1.0f, 1.0f );
#endif
}

CZMViewModel::~CZMViewModel()
{
}

CBaseCombatWeapon* CZMViewModel::GetOwningWeapon()
{
    auto* pOwner = CBaseViewModel::GetOwningWeapon();
    if ( pOwner )
        return pOwner;


    // Arm viewmodel does not have an owning. Ask our brother.
    if ( ViewModelIndex() == VMINDEX_HANDS )
    {
        auto* pPlayer = static_cast<C_ZMPlayer*>( GetOwner() );

        if ( pPlayer )
        {
            CBaseViewModel* vm = pPlayer->GetViewModel( VMINDEX_WEP, false );

            // Apparently this is possible...
            // ???
            if ( vm && vm->ViewModelIndex() == VMINDEX_WEP )
            {
                return vm->GetOwningWeapon();
            }
        }
    }

    return nullptr;
}

void CZMViewModel::SetWeaponModelEx( const char* pszModel, CBaseCombatWeapon* pWep, bool bOverriden )
{
#ifdef CLIENT_DLL
    // Set override model
    auto* pCurWeapon = GetWeapon();

    int newIndex = modelinfo->GetModelIndex( pszModel );

    if ( pWep != pCurWeapon )
    {
        m_iOverrideModelIndex = bOverriden ? newIndex : -1;
        m_pOverrideModelWeapon = pWep;

        m_pLastWeapon = pCurWeapon;
    }

#endif

    SetWeaponModel( pszModel, pWep );
}

void CZMViewModel::CalcViewModelView( CBasePlayer* pOwner, const Vector& eyePosition, const QAngle& eyeAngles )
{
    Vector originalPos = eyePosition;
    QAngle originalAng = eyeAngles;

    Vector newPos = originalPos;
    QAngle newAng = originalAng;

    // Arms are bonemerged. They don't need this fancy stuff.
    if ( ViewModelIndex() == VMINDEX_WEP )
    {
#ifdef CLIENT_DLL
        if ( !prediction->InPrediction() )
        {
            static float flPrevCycle = 0;
            // HACK: We need to manually advance the idle animations because
            // they are no longer interpolated. (pose parameter bobbing)
            // Make sure this is called every frame.
            Activity activity = GetSequenceActivity( GetSequence() );
            if ( activity == ACT_VM_IDLE || activity == ACT_VM_IDLE_EMPTY )
            {
                SetCycle( flPrevCycle );
                FrameAdvance( gpGlobals->frametime );
            }
            flPrevCycle = GetCycle();
            

            PerformAnimBobbing();

            // Let the viewmodel shake at about 10% of the amplitude of the player's view
            vieweffects->ApplyShake( newPos, newAng, 0.1f );

            PerformIronSight( newPos, newAng, originalAng );

            PerformOldBobbing( newPos, newAng );

            PerformLag( newPos, newAng, originalPos, originalAng );
        }
#endif
    }

    SetLocalOrigin( newPos );
    SetLocalAngles( newAng );
}

CZMPlayer* CZMViewModel::GetOwner() const
{
    return static_cast<CZMPlayer*>( CBaseViewModel::GetOwner() );
}

#ifdef CLIENT_DLL
CZMBaseWeapon* CZMViewModel::GetWeapon() const
{
    return static_cast<CZMBaseWeapon*>( CBaseViewModel::GetWeapon() );
}

// Release interpolated stuff.
// This is separate from ResetLatched because it gets called pretty often.
void C_ZMViewModel::OnTeleported()
{
    m_vecLastVel = vec3_origin;
}

bool C_ZMViewModel::IsInIronsights() const
{
    return m_bInIronSight;
}

bool C_ZMViewModel::ShouldDrawCrosshair() const
{
    return !IsInIronsights() || zm_cl_crosshair_draw_ironsights.GetBool();
}

ConVar zm_cl_ironsight_zoom_rate( "zm_cl_ironsight_zoom_rate", "6" );
ConVar zm_cl_ironsight_unzoom_rate( "zm_cl_ironsight_unzoom_rate", "5" );

bool C_ZMViewModel::PerformIronSight( Vector& vecOut, QAngle& angOut, const QAngle& origAng )
{
    if ( ViewModelIndex() != VMINDEX_WEP ) return false;


    if ( m_iAttachmentIronsight <= 0 ) return false;


    auto* pWeapon = C_ZMViewModel::GetWeapon();
    if ( !pWeapon ) return false;


    bool bPrevState = m_bInIronSight;
    bool bChanged = false;
    if ( prediction->IsFirstTimePredicted() )
    {
        bChanged = pWeapon->IsZoomed() != bPrevState;
    }

    if ( bChanged )
    {
        m_bInIronSight = !m_bInIronSight;
    }
    
    if ( m_bInIronSight )
    {
        if ( m_flIronSightFrac != 1.0f )
        {
            m_flIronSightFrac += gpGlobals->frametime * zm_cl_ironsight_zoom_rate.GetFloat();
            m_flIronSightFrac = MIN( m_flIronSightFrac, 1.0f );
        }
    }
    else
    {
        if ( m_flIronSightFrac != 0.0f )
        {
            m_flIronSightFrac -= gpGlobals->frametime * zm_cl_ironsight_unzoom_rate.GetFloat();
            m_flIronSightFrac = MAX( m_flIronSightFrac, 0.0f );
        }
    }


    Vector vecLocal;
    Vector vecIronsightPos;
    Vector vecEyePos = vec3_origin;


    // Get the iron sight position relative to the camera position.
    auto* pHdr = GetModelPtr();
    auto& attachment = pHdr->pAttachment( m_iAttachmentIronsight-1 );

    // The attachment position is local to the bone.
    MatrixPosition( attachment.local, vecLocal );

    mstudiobone_t* pBone = nullptr;
    if ( attachment.localbone >= 0 )
    {
        pBone = pHdr->pBone( attachment.localbone );
    }
    
    if ( pBone )
    {
        vecLocal += pBone->pos;
    }

    
    VectorRotate( vecLocal, origAng, vecIronsightPos );


    //auto smootherstep = []( float x ) { return x * x * x * (x * (x * 6 - 15) + 10); };
    auto inversesquare = [ this ]( float x )
    {
        if ( m_bInIronSight )
        {
            float inv = 1.0f - x;
            return 1.0f - inv * inv;
        }
        else
            return x * x;
    };

    Vector vecCur = Lerp( inversesquare( m_flIronSightFrac ), vecEyePos, vecIronsightPos );
    

    vecOut -= vecCur;

    return true;
}

bool C_ZMViewModel::ShouldRenderScope() const
{
    return m_iAttachmentScopeEnd > 0 && !IsEffectActive( EF_NODRAW );
}

void C_ZMViewModel::GetScopeEndPosition( Vector& pos, QAngle& ang )
{
    if ( m_iAttachmentScopeEnd <= 0 )
        return;


    GetAttachment( m_iAttachmentScopeEnd, pos, ang );
}

ConVar zm_cl_bob_lag_interp( "zm_cl_bob_lag_interp", "0.1" );
ConVar zm_cl_bob_lag_angle_mult( "zm_cl_bob_lag_angle_mult", "0.07" );
ConVar zm_cl_bob_lag_angle_move_mult( "zm_cl_bob_lag_angle_move_mult", "0.01" );
//ConVar zm_cl_bob_lag_movement_fwd_pitch_mult( "zm_cl_bob_lag_movement_fwd_pitch_mult", "0.1" );
ConVar zm_cl_bob_lag_movement_side_roll_mult( "zm_cl_bob_lag_movement_side_roll_mult", "2.5" );
ConVar zm_cl_bob_lag_movement_side_yaw_mult( "zm_cl_bob_lag_movement_side_yaw_mult", "1" );
ConVar zm_cl_bob_lag_movement_side_move_mult( "zm_cl_bob_lag_movement_side_move_mult", "0.5" );

bool C_ZMViewModel::PerformLag( Vector& vecPos, QAngle& ang, const Vector& origPos, const QAngle& origAng )
{
    Vector fwd, right, up;
    AngleVectors( origAng, &fwd, &right, &up );

    // Looking around moves the vm
    PerformAngleLag( vecPos, ang, origAng, right, up );
    
    // Moving around moves the vm
    PerformMovementLag( vecPos, ang, fwd, right );

    // Jumping/crouching/landing moves the vm
    PerformImpactLag( vecPos, ang, origPos );


    NormalizeAngles( ang );


    return true;
}

bool C_ZMViewModel::PerformAngleLag( Vector& vecPos, QAngle& ang, const QAngle& origAng, const Vector& right, const Vector& up )
{
    const float flInterp = zm_cl_bob_lag_interp.GetFloat();


    // Add an entry to the history.
    m_vLagAngles = origAng;
    m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, flInterp, false );
    
    // Interpolate back 100ms.
    m_LagAnglesHistory.Interpolate( gpGlobals->curtime, flInterp );


    QAngle angleDiff = origAng - m_vLagAngles;

    NormalizeAngles( angleDiff );

    ang += angleDiff * zm_cl_bob_lag_angle_mult.GetFloat();

    vecPos -= up * angleDiff.x * zm_cl_bob_lag_angle_move_mult.GetFloat();
    vecPos -= right * angleDiff.y * zm_cl_bob_lag_angle_move_mult.GetFloat();

    return true;
}

bool C_ZMViewModel::PerformMovementLag( Vector& vecPos, QAngle& ang, const Vector& fwd, const Vector& right )
{
    auto* pOwner = GetOwner();
    if ( !pOwner )
        return false;


    if ( IsInIronsights() )
        return false;

    float flMaxGroundSpeed = pOwner->GetPlayerMaxSpeed();
    flMaxGroundSpeed = MAX( flMaxGroundSpeed, 1.0f ); // Please don't divide by 0


    Vector vel = pOwner->GetLocalVelocity();

    // Smooth out the movement a bit.
    vel.x = ApproachSmooth( vel.x, m_vecLastVel.x, 1000.0f * gpGlobals->frametime, 150.0f );
    vel.y = ApproachSmooth( vel.y, m_vecLastVel.y, 1000.0f * gpGlobals->frametime, 150.0f );
    m_vecLastVel = vel;

    Vector vecVelDir = vel;
    vecVelDir.x = vecVelDir.x / flMaxGroundSpeed;
    vecVelDir.y = vecVelDir.y / flMaxGroundSpeed;
    vecVelDir.z = 0.0f;

    // Clamp to 1
    vecVelDir.x = clamp( vecVelDir.x, -1.0f, 1.0f );
    vecVelDir.y = clamp( vecVelDir.y, -1.0f, 1.0f );

    //float dotFwd = fwd.Dot( vecVelDir );
    float dotRight = right.Dot( vecVelDir );


    //ang.x += dotFwd * zm_cl_bob_lag_movement_fwd_pitch_mult.GetFloat();
    ang.y -= dotRight * zm_cl_bob_lag_movement_side_yaw_mult.GetFloat();
    ang.z -= dotRight * zm_cl_bob_lag_movement_side_roll_mult.GetFloat();

    vecPos += right * dotRight * zm_cl_bob_lag_movement_side_move_mult.GetFloat();

    return true;
}


ConVar zm_cl_bob_lag_impact_approachspeed( "zm_cl_bob_lag_impact_approachspeed", "120" );
ConVar zm_cl_bob_lag_impact_approachsmooth( "zm_cl_bob_lag_impact_approachsmooth", "24" );

ConVar zm_cl_bob_lag_impact_air_angle( "zm_cl_bob_lag_impact_air_angle", "4" );
ConVar zm_cl_bob_lag_impact_air_move( "zm_cl_bob_lag_impact_air_move", "1" );
ConVar zm_cl_bob_lag_impact_air_vel_rate( "zm_cl_bob_lag_impact_air_vel_rate", "0.1" );
ConVar zm_cl_bob_lag_impact_air_vel_max( "zm_cl_bob_lag_impact_air_vel_max", "300" );

ConVar zm_cl_bob_lag_impact_ground_angle( "zm_cl_bob_lag_impact_ground_angle", "5" );
ConVar zm_cl_bob_lag_impact_ground_move( "zm_cl_bob_lag_impact_ground_move", "1" );

ConVar zm_cl_bob_lag_impact_rate( "zm_cl_bob_lag_impact_rate", "0.1" );

bool C_ZMViewModel::PerformImpactLag( Vector& vecPos, QAngle& ang, const Vector& origPos )
{
    auto* pOwner = GetOwner();
    if ( !pOwner )
        return false;



    bool bOnGround = (pOwner->GetFlags() & FL_ONGROUND) != 0;


    float flBaseValue = pOwner->GetViewOffset().z;

    if ( !bOnGround )
    {
        // The view offset changes jarringly in the air when crouching/uncrouching.
        flBaseValue = m_flLastImpactGroundOffsetZ;
    }
    else
    {
        // Once we're back on ground, translate it.
        if ( !m_bWasOnGround )
        {
            m_flLastImpactValue += flBaseValue - m_flLastImpactGroundOffsetZ;
        }

        m_flLastImpactGroundOffsetZ = flBaseValue;
    }

    float flCurValue = flBaseValue;

    if ( !bOnGround )
    {
        if ( pOwner->GetMoveType() != MOVETYPE_NOCLIP )
        {
            float maxvelocity = zm_cl_bob_lag_impact_air_vel_max.GetFloat();
            float vel = clamp( pOwner->GetLocalVelocity().z, -maxvelocity, maxvelocity );

            flCurValue -= vel * zm_cl_bob_lag_impact_air_vel_rate.GetFloat();
        }
    }


    // Approach the value smoothly
    float flNewValue = ApproachSmooth(
        flCurValue,
        m_flLastImpactValue,
        gpGlobals->frametime * zm_cl_bob_lag_impact_approachspeed.GetFloat(),
        zm_cl_bob_lag_impact_approachsmooth.GetFloat() );


    float delta = flBaseValue - flNewValue;

    float flAngleValue = delta;
    float flMoveValue = delta;

    float flAngleMax;
    float flMoveMax;

    float flRate = zm_cl_bob_lag_impact_rate.GetFloat();
    

    if ( bOnGround )
    {
        flAngleMax = fabsf( zm_cl_bob_lag_impact_ground_angle.GetFloat() );
        flMoveMax = fabsf( zm_cl_bob_lag_impact_ground_move.GetFloat() );
    }
    else
    {
        flAngleMax = fabsf( zm_cl_bob_lag_impact_air_angle.GetFloat() );
        flMoveMax = fabsf( zm_cl_bob_lag_impact_air_move.GetFloat() );
    }


    flAngleValue *= flRate;
    flMoveValue *= flRate;

    flAngleValue = clamp( flAngleValue, -flAngleMax, flAngleMax );
    flMoveValue = clamp( flMoveValue, -flMoveMax, flMoveMax );
    
    ang.x += flAngleValue;
    vecPos.z -= flMoveValue;


    m_flLastImpactValue = flNewValue;
    m_bWasOnGround = bOnGround;

    return true;
}

bool C_ZMViewModel::CanAnimBob() const
{
    return m_iPoseParamMoveX != -1;
}

// Version of cl_bob* cvars that are actually useful...
ConVar cl_bobcycle( "cl_bobcycle", "0.6", 0 , "How fast the bob cycles", true, 0.01f, false, 0.0f );
ConVar cl_bobup( "cl_bobup", "0.5", 0 , "Don't change...", true, 0.01f, true, 0.99f );
ConVar cl_bobvertscale( "cl_bobvertscale", "0.6", 0, "Vertical scale" ); // Def. is 0.1
ConVar cl_boblatscale( "cl_boblatscale", "0.8", 0, "Lateral scale" );
ConVar cl_bobenable( "cl_bobenable", "1" );


bool C_ZMViewModel::PerformOldBobbing( Vector& vecPos, QAngle& ang )
{
    // We're going to be doing the animation bobbing.
    // Skip us.
    if ( CanAnimBob() )
    {
        return false;
    }


    if ( IsInIronsights() )
        return false;

    if ( !cl_bobenable.GetBool() )
        return false;


    float bobup = cl_bobup.GetFloat();
    float bobcycle = cl_bobcycle.GetFloat();
    

    auto* pOwner = GetOwner();


    if ( !pOwner || bobcycle <= 0.0f || bobup <= 0.0f || bobup >= 1.0f )
    {
        return false;
    }


    static float bobtime = 0.0f;
    static float lastbobtime = 0.0f;
    float cycle;

    float verticalBob;
    float lateralBob;


    const float flMaxGroundSpeed = pOwner->GetPlayerMaxSpeed();


    float speed = pOwner->GetLocalVelocity().Length2D();
    speed = clamp( speed, 0.0f, flMaxGroundSpeed*2.0f ); // Clamp the speed a bit so it doesn't look terrible.
    float bob_offset = RemapValClamped( speed, 0.0f, flMaxGroundSpeed, 0.0f, 1.0f );
    
    bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
    lastbobtime = gpGlobals->curtime;

    // Calculate the vertical bob
    cycle = bobtime - (int)(gpGlobals->curtime/bobcycle)*bobcycle;
    cycle /= bobcycle;

    if ( cycle < bobup )
    {
        cycle = M_PI_F * cycle / bobup;
    }
    else
    {
        cycle = M_PI_F + M_PI_F*(cycle-bobup)/(1.0f - bobup);
    }
    
    verticalBob = speed*0.005f;
    verticalBob = verticalBob*0.3f + verticalBob*0.7f*sinf(cycle);

    verticalBob = clamp( verticalBob, -7.0f, 4.0f );

    // Calculate the lateral bob
    cycle = bobtime - (int)(bobtime/bobcycle*2)*bobcycle*2;
    cycle /= bobcycle*2;

    if ( cycle < bobup )
    {
        cycle = M_PI_F * cycle / bobup;
    }
    else
    {
        cycle = M_PI_F + M_PI_F*(cycle-bobup)/(1.0f - bobup);
    }

    
    lateralBob = speed*0.005f;
    lateralBob = lateralBob*0.3f + lateralBob*0.7f*sinf(cycle);
    lateralBob = clamp( lateralBob, -7.0f, 4.0f );



    Vector	fwd, right;
    AngleVectors( ang, &fwd, &right, nullptr );

    // Apply bob, but scaled down to 40%
    VectorMA( vecPos, verticalBob * cl_bobvertscale.GetFloat(), fwd, vecPos );
    
    // Z bob a bit more
    vecPos.z += verticalBob * 0.1f;
    
    // bob the angles
    ang.z += verticalBob * 0.5f; // Roll
    ang.x -= verticalBob * 0.4f; // Pitch

    ang.y -= lateralBob * 0.3f; // Yaw


    VectorMA( vecPos, lateralBob * cl_boblatscale.GetFloat(), right, vecPos );

    return true;
}

ConVar zm_cl_bob_anim_accel( "zm_cl_bob_anim_accel", "1" );
ConVar zm_cl_bob_anim_decel( "zm_cl_bob_anim_decel", "1.6" );
ConVar zm_cl_bob_ironsight_max( "zm_cl_bob_ironsight_max", "0.1" );

void C_ZMViewModel::PerformAnimBobbing()
{
    auto* pOwner = GetOwner();
    if ( !pOwner ) return;



    //
    // Bobbing
    //
    if ( m_iPoseParamMoveX != -1 )
    {
        float flMaxGroundSpeed = pOwner->GetPlayerMaxSpeed();
        flMaxGroundSpeed = MAX( flMaxGroundSpeed, 1.0f ); // Please don't divide by 0


        float spd = pOwner->GetLocalVelocity().Length2D();
        float target = spd > 0.1f ? spd / flMaxGroundSpeed : 0.0f;

        float max = 1.0f;

        if ( IsInIronsights() )
            max = zm_cl_bob_ironsight_max.GetFloat();

        target = clamp( target, 0.0f, max );

        //
        // The pose parameter goes from:
        // Backwards, no movement, forwards
        // In numbers:
        // -1.0 .. 0.0 .. 1
        //
        // But when we retrieve the actual pose parameter it will be:
        //  0.0 .. 0.5 .. 1
        //
        // When setting the pose parameter, we need to use former scale.
        // 

        //float cur = GetPoseParameter( m_iPoseParamMoveX );
        //cur = clamp( cur, 0.5f, 1.0f );

        //// Translate from
        //// 0.5 .. 1 to 0 .. 1
        //cur /= 0.5f;
        //cur -= 1.0f;

        float cur = m_flLastMoveX;


        float add = 0.0f;
        if ( target > cur )
        {
            add = zm_cl_bob_anim_accel.GetFloat();
        }
        else if ( target < cur )
        {
            add = -zm_cl_bob_anim_decel.GetFloat();
        }


        float newratio = cur + gpGlobals->frametime * add;
        newratio = clamp( newratio, 0.0f, 1.0f );

        SetPoseParameter( m_iPoseParamMoveX, newratio );
        m_flLastMoveX = newratio;
    }

    //
    // Vertical aiming stuff (unused)
    //
    if ( m_iPoseParamVertAim != -1 )
    {
        float vert = pOwner->EyeAngles().x;
        vert = clamp( vert, -90.0f, 90.0f );
        vert /= 90.0f;

        SetPoseParameter( m_iPoseParamVertAim, vert );
    }
}

int C_ZMViewModel::CalcOverrideModelIndex()
{
    if ( m_iOverrideModelIndex != -1 )
    {
        // HACK: Check if we changed weapons.
        auto* pCurWeapon = GetWeapon();
        if ( pCurWeapon != m_pOverrideModelWeapon && pCurWeapon != m_pLastWeapon )
        {
            // Stop overriding.
            m_iOverrideModelIndex = -1;
            m_pOverrideModelWeapon = nullptr;
        }
    }


    return m_iOverrideModelIndex;
}

CStudioHdr* C_ZMViewModel::OnNewModel()
{
    auto* pHdr = BaseClass::OnNewModel();

    m_iPoseParamMoveX = LookupPoseParameter( "move_x" );
    m_iPoseParamVertAim = LookupPoseParameter( "ver_aims" );

    m_iAttachmentIronsight = LookupAttachment( "ironsight" );
    m_iAttachmentScopeEnd = LookupAttachment( "scope_end" );


    m_LagAnglesHistory.ClearHistory();


    return pHdr;
}

int C_ZMViewModel::DrawModel( int flags )
{
    if ( m_bDrawVM )
    {
        return BaseClass::DrawModel( flags );
    }

    return 0;
}

bool C_ZMViewModel::ShouldReceiveProjectedTextures( int flags )
{
    //
    // IMPORTANT: There's a common crash is caused by things that return true in ShouldReceiveProjectedTextures
    // I've not been able to debug it, so fuck it.
    //
    return false;
}

C_BaseAnimating* C_ZMViewModel::FindFollowedEntity()
{
    if ( ViewModelIndex() == VMINDEX_HANDS )
    {
        C_ZMPlayer* pPlayer = static_cast<C_ZMPlayer*>( GetOwner() );

        if ( pPlayer )
        {
            C_BaseViewModel* vm = pPlayer->GetViewModel( VMINDEX_WEP );

            if ( vm )
            {
                return vm;
            }
        }
    }

    return C_BaseAnimating::FindFollowedEntity();
}

bool C_ZMViewModel::ShouldPredict()
{
    if ( GetOwner() && GetOwner() == C_ZMPlayer::GetLocalPlayer() )
        return true;
    
    return BaseClass::ShouldPredict();
}

class CViewModelColorMaterialProxy : public CEntityMaterialProxy
{
public:
    CViewModelColorMaterialProxy();
    ~CViewModelColorMaterialProxy();

    virtual bool Init( IMaterial* pMaterial, KeyValues* pKeyValues ) OVERRIDE;
    virtual IMaterial* GetMaterial() OVERRIDE;

private:
    virtual void OnBind( C_BaseEntity* pEntity ) OVERRIDE;


    IMaterialVar*   m_defaultVar;
    float           m_defR;
    float           m_defG;
    float           m_defB;
};

CViewModelColorMaterialProxy::CViewModelColorMaterialProxy()
{
    m_defaultVar = nullptr;
    m_defR = m_defG = m_defB = 1.0f;
}

CViewModelColorMaterialProxy::~CViewModelColorMaterialProxy()
{
}

void UTIL_ParseFloatColorFromString( const char* str, float clr[], int nColors );

bool CViewModelColorMaterialProxy::Init( IMaterial* pMaterial, KeyValues* pKeyValues )
{
    bool foundVar;
    m_defaultVar = pMaterial->FindVar( pKeyValues->GetString( "resultVar", "" ), &foundVar, false );


    float clr[3];
    UTIL_ParseFloatColorFromString( pKeyValues->GetString( "default", "" ), clr, ARRAYSIZE( clr ) );

    m_defR = clr[0];
    m_defG = clr[1];
    m_defB = clr[2];

    return foundVar;
}

void CViewModelColorMaterialProxy::OnBind( C_BaseEntity* pEnt )
{
    if ( !m_defaultVar )
        return;


    float r, g, b;
    r = m_defR;
    g = m_defG;
    b = m_defB;


    C_ZMViewModel* pVM = static_cast<C_ZMViewModel*>( pEnt );
    if ( pVM )
    {
        pVM->GetModelColor2( r, g, b );
    }


    m_defaultVar->SetVecValue( r, g, b );
}

IMaterial* CViewModelColorMaterialProxy::GetMaterial()
{
    if ( !m_defaultVar )
        return nullptr;

    return m_defaultVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CViewModelColorMaterialProxy, IMaterialProxy, "ViewModelColor" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//
// Approach a value with inverse square smoothing.
//
static float ApproachSmooth( float target, float value, float speed, float smoothness_start, float epsilon )
{
    float dif, f;
    if ( target < value )
    {
        dif = value - target;

        if ( dif < epsilon )
            return target;


        f = dif / smoothness_start;

        if ( f < 1.0f )
        {
            f = 1 - f;
            speed *= 1 - f*f;
        }

        float newvalue = value - speed;
        return MAX( newvalue, target );
    }
    else
    {
        dif = target - value;

        if ( dif < epsilon )
            return target;


        f = dif / smoothness_start;

        if ( f < 1.0f )
        {
            f = 1 - f;
            speed *= 1 - f*f;
        }

        float newvalue = value + speed;
        return MIN( newvalue, target );
    }
}
