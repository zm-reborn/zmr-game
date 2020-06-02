#include "cbase.h"


#include "zmr/zmr_player_shared.h"
#include "zmr_viewmodel.h"

#ifdef CLIENT_DLL
#include <materialsystem/imaterialvar.h>
#include "proxyentity.h"
#include "ivieweffects.h"
#include "prediction.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


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

    SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),
    SendPropExclude( "DT_BaseViewModel", "m_flPlaybackRate" ),

    //SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_ZMViewModel )

    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_flAnimTime, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_ARRAY_TOL( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK, 0.0f ),

    DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK, 0.02f ),

END_PREDICTION_DATA()
#endif

#ifdef CLIENT_DLL
CZMViewModel::CZMViewModel() : m_LagAnglesHistory( "CZMViewModel::m_LagAnglesHistory" ), m_flLagEyePosZHistory( "CZMViewModel::m_flLagEyePosZHistory")
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
    m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );

    m_flLagEyePosZ = 0.0f;
    m_flLagEyePosZHistory.Setup( &m_flLagEyePosZ, 0 );

    m_flLastImpactDelta = 0.0f;
    m_flImpactTargetDelta = 0.0f;
    m_flImpactVel = 0.0f;

    m_iPoseParamMoveX = -1;
    m_iPoseParamVertAim = -1;
#else
    SetModelColor2( 1.0f, 1.0f, 1.0f );
#endif

    UseClientSideAnimation();
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
        // Let the viewmodel shake at about 10% of the amplitude of the player's view
        vieweffects->ApplyShake( newPos, newAng, 0.1f );

        PerformIronSight( newPos, newAng, originalAng );

        PerformOldBobbing( newPos, newAng );

        PerformLag( newPos, newAng, originalPos, originalAng );
#endif
    }

    SetLocalOrigin( newPos );
    SetLocalAngles( newAng );
}

#ifdef CLIENT_DLL
void C_ZMViewModel::UpdateClientSideAnimation()
{
    PerformAnimBobbing();

    BaseClass::UpdateClientSideAnimation();
}

bool C_ZMViewModel::ShouldPredict()
{
    auto* pOwner = GetOwner();
    if ( pOwner && pOwner == C_ZMPlayer::GetLocalPlayer() )
        return true;

    return BaseClass::ShouldPredict();
}

bool C_ZMViewModel::Interpolate( float currentTime )
{
    // We need to skip the C_BaseViewModel interpolation as it fucks up our client-side cycle.
    return C_BaseAnimating::Interpolate( currentTime );
}

bool C_ZMViewModel::IsInIronsights() const
{
    return m_bInIronSight;
}

ConVar zm_cl_ironsight_zoom_rate( "zm_cl_ironsight_zoom_rate", "1.2" );
ConVar zm_cl_ironsight_unzoom_rate( "zm_cl_ironsight_unzoom_rate", "1.2" );

bool C_ZMViewModel::PerformIronSight( Vector& vecOut, QAngle& angOut, const QAngle& origAng )
{
    if ( ViewModelIndex() != VMINDEX_WEP ) return false;


    if ( m_iAttachmentIronsight <= 0 ) return false;


    auto* pWeapon = C_ZMViewModel::GetWeapon();

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
    // Here we are assuming that the bone is at origin.
    MatrixPosition( attachment.local, vecLocal );

    
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


ConVar zm_cl_bob_lag_interp( "zm_cl_bob_lag_interp", "0.1" );
ConVar zm_cl_bob_lag_angle_mult( "zm_cl_bob_lag_angle_mult", "0.1" );
//ConVar zm_cl_bob_lag_movement_fwd_pitch_mult( "zm_cl_bob_lag_movement_fwd_pitch_mult", "0.1" );
ConVar zm_cl_bob_lag_movement_side_roll_mult( "zm_cl_bob_lag_movement_side_roll_mult", "3" );
ConVar zm_cl_bob_lag_movement_side_yaw_mult( "zm_cl_bob_lag_movement_side_yaw_mult", "2" );

bool C_ZMViewModel::PerformLag( Vector& vecPos, QAngle& ang, const Vector& origPos, const QAngle& origAng )
{
    Vector fwd, right;
    AngleVectors( origAng, &fwd, &right, nullptr );

    // Looking around moves the vm
    PerformAngleLag( vecPos, ang, origAng );
    
    // Moving around moves the vm
    PerformMovementLag( vecPos, ang, fwd, right );

    // Jumping/crouching/landing moves the vm
    PerformImpactLag( vecPos, ang, origPos );


    NormalizeAngles( ang );


    return true;
}

bool C_ZMViewModel::PerformAngleLag( Vector& vecPos, QAngle& ang, const QAngle& origAng )
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

    return true;
}

ConVar zm_cl_bob_lag_impact_interp( "zm_cl_bob_lag_impact_interp", "0.1" );

ConVar zm_cl_bob_lag_impact_air( "zm_cl_bob_lag_impact_air", "4" );
ConVar zm_cl_bob_lag_impact_air_rate( "zm_cl_bob_lag_impact_air_rate", "0.2" );

ConVar zm_cl_bob_lag_impact_ground( "zm_cl_bob_lag_impact_ground", "5" );
ConVar zm_cl_bob_lag_impact_ground_rate( "zm_cl_bob_lag_impact_ground_rate", "0.15" );

ConVar zm_cl_bob_lag_impact_land( "zm_cl_bob_lag_impact_land", "250" );
ConVar zm_cl_bob_lag_impact_land_rate( "zm_cl_bob_lag_impact_land_rate", "30" );

bool C_ZMViewModel::PerformImpactLag( Vector& vecPos, QAngle& ang, const Vector& origPos )
{
    auto* pOwner = GetOwner();
    if ( !pOwner )
        return false;


    const float flInterp = zm_cl_bob_lag_impact_interp.GetFloat();


    bool bOnGround = (pOwner->GetFlags() & FL_ONGROUND) != 0;


    // Add an entry to history.
    m_flLagEyePosZ = origPos.z;
    m_flLagEyePosZHistory.NoteChanged( gpGlobals->curtime, flInterp, false );

    // Interpolate back 100ms.
    m_flLagEyePosZHistory.Interpolate( gpGlobals->curtime, flInterp );


    float delta = origPos.z - m_flLagEyePosZ;

    
    
    if ( bOnGround )
    {
        float maxvalue = fabsf( zm_cl_bob_lag_impact_ground.GetFloat() );
        delta *= zm_cl_bob_lag_impact_ground_rate.GetFloat();
        delta = clamp( delta, -maxvalue, maxvalue );
    }
    else
    {
        float maxvalue = fabsf( zm_cl_bob_lag_impact_air.GetFloat() );
        delta *= zm_cl_bob_lag_impact_air_rate.GetFloat();
        delta = clamp( delta, -maxvalue, maxvalue );
    }

    float flOriginalDelta = delta;


    float flNextDelta = flOriginalDelta;
    
    
    // Jumping/landing
    if ( bOnGround != m_bOnGround )
    {
        m_bOnGround = bOnGround;
        m_flGroundTime = gpGlobals->curtime;

        if ( bOnGround )
        {
            // We landed, init the impact logic.
            const float flMaxLandingSpeed = 400.0f;


            float impactSpd = fabsf( delta ) / flInterp;
            impactSpd = MAX( impactSpd, 0.0f );

            float mult = impactSpd / flMaxLandingSpeed;
            mult = MIN( mult, 1.0f );

            m_flImpactTargetDelta = m_flLastImpactDelta;
            m_flImpactVel = zm_cl_bob_lag_impact_land.GetFloat() * mult;
            m_flImpactVelOrig = m_flImpactVel;
        }
    }

    
    if ( bOnGround )
    {
        // We're doing the impact effect.
        if ( m_flImpactVel > 0.0f || m_flImpactTargetDelta > flOriginalDelta )
        {
            m_flImpactVel -= zm_cl_bob_lag_impact_land_rate.GetFloat() * gpGlobals->frametime;

            // Still above our intended spot, maintain velocity.
            if ( m_flImpactTargetDelta < flOriginalDelta )
            {
                m_flImpactVel = MAX( m_flImpactVel, m_flImpactVelOrig );
            }

            m_flImpactTargetDelta += m_flImpactVel * gpGlobals->frametime;
            flNextDelta = m_flImpactTargetDelta;
        }
    }

    ang.x += flNextDelta;

    if ( !bOnGround ) // Save our last delta for the impact effect.
    {
        m_flLastImpactDelta = flNextDelta;
    }

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
ConVar zm_cl_bob_anim_decel( "zm_cl_bob_anim_decel", "1.2" );

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
        target = clamp( target, 0.0f, 1.0f );

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

        float cur = GetPoseParameter( m_iPoseParamMoveX );
        cur = clamp( cur, 0.5f, 1.0f );

        // Translate from
        // 0.5 .. 1 to 0 .. 1
        cur /= 0.5f;
        cur -= 1.0f;


        float add = 0.0f;
        if ( target > cur )
        {
            add = zm_cl_bob_anim_accel.GetFloat();
        }
        else if ( target < cur )
        {
            add = -zm_cl_bob_anim_decel.GetFloat();
        }

        if ( add != 0.0f )
        {
            float newratio = cur + gpGlobals->frametime * add;

            SetPoseParameter( m_iPoseParamMoveX, clamp( newratio, 0.0f, 1.0f ) );
        }
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
