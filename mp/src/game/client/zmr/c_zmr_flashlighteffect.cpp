#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "iviewrender_beams.h"

#include "c_zmr_player.h"
#include "c_zmr_flashlightsystem.h"
#include "c_zmr_flashlighteffect.h"
#include "zmr/zmr_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar zm_cl_flashlight_useshadows( "zm_cl_flashlight_useshadows", "2", FCVAR_ARCHIVE );
ConVar zm_cl_flashlight_thirdperson( "zm_cl_flashlight_thirdperson", "1", FCVAR_ARCHIVE, "Do other players have a flashlight beam + dynamic light" );
ConVar zm_cl_flashlight_spec_uselocal( "zm_cl_flashlight_spec_uselocal", "1", FCVAR_ARCHIVE, "Use better flashlight when spectating in firstperson" );
ConVar zm_cl_flashlight_expensive_fadetime( "zm_cl_flashlight_expensive_fadetime", "0.4", FCVAR_ARCHIVE );



extern ConVar r_flashlightdepthres;
extern ConVar r_flashlightdepthtexture;

static ConVar r_swingflashlight( "r_swingflashlight", "1", FCVAR_CHEAT );
static ConVar r_flashlightlockposition( "r_flashlightlockposition", "0", FCVAR_CHEAT );
static ConVar r_flashlightfov( "r_flashlightfov", "45.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetx( "r_flashlightoffsetx", "10.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsety( "r_flashlightoffsety", "-20.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetz( "r_flashlightoffsetz", "24.0", FCVAR_CHEAT );
static ConVar r_flashlightnear( "r_flashlightnear", "4.0", FCVAR_CHEAT );
static ConVar r_flashlightfar( "r_flashlightfar", "750.0", FCVAR_CHEAT );
static ConVar r_flashlightconstant( "r_flashlightconstant", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightlinear( "r_flashlightlinear", "100.0", FCVAR_CHEAT );
static ConVar r_flashlightquadratic( "r_flashlightquadratic", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightvisualizetrace( "r_flashlightvisualizetrace", "0", FCVAR_CHEAT );
static ConVar r_flashlightambient( "r_flashlightambient", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightshadowatten( "r_flashlightshadowatten", "0.35", FCVAR_CHEAT );
static ConVar r_flashlightladderdist( "r_flashlightladderdist", "40.0", FCVAR_CHEAT );
static ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "16", FCVAR_CHEAT );
static ConVar mat_depthbias_shadowmap(	"mat_depthbias_shadowmap", "0.0005", FCVAR_CHEAT  );


CZMFlashlightEffect::CZMFlashlightEffect( CZMPlayer* pPlayer )
{
    m_pPlayer = pPlayer;
    Assert( m_pPlayer );


    m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
    m_bIsOn = false;


    m_pFlashlightBeam = nullptr;
    m_iAttachmentRH = m_pPlayer->LookupAttachment( "anim_attachment_RH" );


    m_bPreferExpensive = false;
    m_bDoFade = false;
    m_flFadeAlpha = 1.0f;


    if ( g_pMaterialSystemHardwareConfig->SupportsBorderColor() )
    {
        m_FlashlightTexture.Init( "effects/flashlight_border", TEXTURE_GROUP_OTHER, true );
    }
    else
    {
        m_FlashlightTexture.Init( "effects/flashlight001", TEXTURE_GROUP_OTHER, true );
    }


    if ( !m_pPlayer->IsLocalPlayer() )
    {
        bool res = GetZMFlashlightSystem()->AddPlayerToExpensiveList( m_pPlayer );
        if ( res )
        {
            PreferExpensive( true );
        }
    }
}

CZMFlashlightEffect::~CZMFlashlightEffect()
{
    LightOff();
}

void CZMFlashlightEffect::SetFlashlightHandle( ClientShadowHandle_t handle )
{
    m_FlashlightHandle = handle;
}

void CZMFlashlightEffect::SetRightHandAttachment( int index )
{
    m_iAttachmentRH = index;
}

void CZMFlashlightEffect::PreferExpensive( bool state )
{
    if ( m_bPreferExpensive != state )
    {
        m_bPreferExpensive = state;

        m_bDoFade = true;
    }
}

void CZMFlashlightEffect::TurnOn()
{
    m_bIsOn = true;
}

void CZMFlashlightEffect::TurnOff()
{
    if ( m_bIsOn )
    {
        m_bIsOn = false;
        LightOff();
    }
}

bool CZMFlashlightEffect::ShouldUseProjected() const
{
    return engine->GetDXSupportLevel() > 70;
}

bool CZMFlashlightEffect::IsInThirdperson() const
{
    C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();
    if ( pLocal == m_pPlayer )
    {
        return !C_ZMPlayer::LocalPlayerInFirstPersonView();
    }
    
    if ( pLocal )
    {
        return pLocal->GetObserverTarget() != m_pPlayer || pLocal->GetObserverMode() != OBS_MODE_IN_EYE;
    }

    return false;
}

bool CZMFlashlightEffect::LocalPlayerWatchingMe() const
{
    C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();
    if ( pLocal == m_pPlayer )
    {
        return true;
    }
    
    if ( pLocal && pLocal->GetObserverMode() == OBS_MODE_IN_EYE && pLocal->GetObserverTarget() == m_pPlayer )
    {
        return zm_cl_flashlight_spec_uselocal.GetBool();
    }

    return false;
}

float CZMFlashlightEffect::GetFlashlightFov() const
{
    return r_flashlightfov.GetFloat();
}

float CZMFlashlightEffect::GetFlashlightNearZ() const
{
    return r_flashlightnear.GetFloat();
}

float CZMFlashlightEffect::GetFlashlightFarZ() const
{
    return r_flashlightfar.GetFloat();
}

bool CZMFlashlightEffect::UseThirdpersonEffects()
{
    return zm_cl_flashlight_thirdperson.GetBool();
}

// Custom trace filter that skips the player and the view model.
// If we don't do this, we'll end up having the light right in front of us all
// the time.
class CTraceFilterSkipPlayerAndViewModel : public CTraceFilter
{
public:
    virtual bool ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask )
    {
        // Test against the vehicle too?
        // FLASHLIGHTFIXME: how do you know that you are actually inside of the vehicle?
        C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
        if ( !pEntity )
            return true;

        if ( ( dynamic_cast<C_BaseViewModel *>( pEntity ) != nullptr ) ||
             pEntity->IsPlayer() ||
             pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
             pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
        {
            return false;
        }

        return true;
    }
};

void CZMFlashlightEffect::UpdateLightNew( const Vector& vecPos, const Vector& vecForward, const Vector& vecRight, const Vector& vecUp, bool bIsThirdperson )
{
    VPROF_BUDGET( "CZMFlashlightEffect::UpdateLightNew", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );


    // We will lock some of the flashlight params if player is on a ladder, to prevent oscillations due to the trace-rays
    //bool bPlayerOnLadder = m_pPlayer->GetMoveType() == MOVETYPE_LADDER;

    //const float flEpsilon = 0.1f;			// Offset flashlight position along vecUp
    //const float flDistCutoff = 128.0f;
    //const float flDistDrag = 0.2f;

    float flFov = GetFlashlightFov();
    float flLinearAtten = r_flashlightlinear.GetFloat();

    CTraceFilterSkipPlayerAndViewModel traceFilter;
    float flOffsetY = r_flashlightoffsety.GetFloat();

    const bool bUseFirstpersonEffects = !bIsThirdperson;


    // How much we will move forward.
    float flMoveFwd = bUseFirstpersonEffects ? 0.0f : 16.0f;

    if ( r_swingflashlight.GetBool() )
    {
        // This projects the view direction backwards, attempting to raise the vertical
        // offset of the flashlight, but only when the player is looking down.
        Vector vecSwingLight = vecPos + vecForward * -12.0f;
        if( vecSwingLight.z > vecPos.z )
        {
            flOffsetY += (vecSwingLight.z - vecPos.z);
        }
    }

    Vector vOrigin = vecPos;// + flOffsetY * vecUp;

    // Not on ladder...trace a hull
    //if ( !bPlayerOnLadder ) 
    //{
    //    trace_t pmOriginTrace;
    //    UTIL_TraceHull( vecPos, vOrigin, Vector(-4, -4, -4), Vector(4, 4, 4), MASK_SOLID & ~(CONTENTS_HITBOX), &traceFilter, &pmOriginTrace );

    //    if ( pmOriginTrace.DidHit() )
    //    {
    //        vOrigin = vecPos;
    //    }
    //}
    //else // on ladder...skip the above hull trace
    //{
    //    vOrigin = vecPos;
    //}

    // Now do a trace along the flashlight direction to ensure there is nothing within range to pull back from
    int iMask = MASK_OPAQUE_AND_NPCS;
    iMask &= ~CONTENTS_HITBOX;
    iMask |= CONTENTS_WINDOW;

    Vector vTarget = vecPos + vecForward * GetFlashlightFarZ();

    // Work with these local copies of the basis for the rest of the function
    Vector vDir   = vecForward;
    Vector vRight = vecRight;
    Vector vUp    = vecUp;
    //VectorNormalize( vDir );
    //VectorNormalize( vRight );
    //VectorNormalize( vUp );

    // Orthonormalize the basis, since the flashlight texture projection will require this later...
    vUp -= DotProduct( vDir, vUp ) * vDir;
    VectorNormalize( vUp );
    vRight -= DotProduct( vDir, vRight ) * vDir;
    VectorNormalize( vRight );
    vRight -= DotProduct( vUp, vRight ) * vUp;
    VectorNormalize( vRight );

    AssertFloatEquals( DotProduct( vDir, vRight ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vDir, vUp    ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vRight, vUp  ), 0.0f, 1e-3 );


    trace_t pmDirectionTrace;
    UTIL_TraceHull( vOrigin, vTarget, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), iMask, &traceFilter, &pmDirectionTrace );

    if ( r_flashlightvisualizetrace.GetBool() )
    {
        debugoverlay->AddBoxOverlay( pmDirectionTrace.endpos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), QAngle( 0, 0, 0 ), 0, 0, 255, 16, 0 );
        debugoverlay->AddLineOverlay( vOrigin, pmDirectionTrace.endpos, 255, 0, 0, false, 0 );
    }

    //float flDist = (pmDirectionTrace.endpos - vOrigin).Length();
    //if ( flDist < flDistCutoff )
    //{
    //    // We have an intersection with our cutoff range
    //    // Determine how far to pull back, then trace to see if we are clear
    //    float flPullBackDist = bPlayerOnLadder ? r_flashlightladderdist.GetFloat() : flDistCutoff - flDist;	// Fixed pull-back distance if on ladder
    //    flMoveFwd = Lerp( flDistDrag, flMoveFwd, flPullBackDist );
    //    
    //    if ( !bPlayerOnLadder )
    //    {
    //        trace_t pmBackTrace;
    //        UTIL_TraceHull( vOrigin, vOrigin - vDir*(flPullBackDist-flEpsilon), Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), iMask, &traceFilter, &pmBackTrace );
    //        if( pmBackTrace.DidHit() )
    //        {
    //            // We have an intersection behind us as well, so limit our m_flMoveFwd
    //            float flMaxDist = (pmBackTrace.endpos - vOrigin).Length() - flEpsilon;
    //            if( flMoveFwd > flMaxDist )
    //                flMoveFwd = flMaxDist;
    //        }
    //    }
    //}
    //else
    //{
    //    flMoveFwd = Lerp( flDistDrag, flMoveFwd, 0.0f );
    //}

    vOrigin = vOrigin + vDir * flMoveFwd;



    //
    // Flickering
    //

    // The battery is networked to local player only.
    if ( m_pPlayer->IsLocalPlayer() && m_pPlayer->GetFlashlightBattery() <= 15.0f )
    {
        float flScale = SimpleSplineRemapVal( m_pPlayer->GetFlashlightBattery(), 10.0f, 4.8f, 1.0f, 0.0f );
        flScale = clamp( flScale, 0.0f, 1.0f );

        if ( flScale < 0.35f )
        {
            float flFlicker = cosf( gpGlobals->curtime * 6.0f ) * sinf( gpGlobals->curtime * 15.0f );
            
            // On/Off
            flLinearAtten = ( flFlicker > 0.25f && flFlicker < 0.75f )
                            ? (flLinearAtten * flScale)
                            : 0.0f;
        }
        else
        {
            float flNoise = cosf( gpGlobals->curtime * 7.0f ) * sinf( gpGlobals->curtime * 25.0f );
            flLinearAtten *= flScale + 1.5f * flNoise;
        }

        flFov -= ( 16.0f * (1.0f-flScale) );
    }



    //
    // TODO: Fix me.
    //

    // Only draw shadows if we're in thirdperson.
    // Honestly, there's no point in drawing the shadows when in first person.
    // You can barely even see the shadows.
    bool bDrawShadows = IsInThirdperson();

    switch ( zm_cl_flashlight_useshadows.GetInt() )
    {
    case 1 : // Only if local player.
        //bDrawShadows = bUseFirstpersonEffects;
        break;
    case 2 : // Always
        //bDrawShadows = true;
        break;
    default : // 0, Never
        //bDrawShadows = false;
        bDrawShadows = false;
        break;
    }


    float clr = 1.0f;


    //
    // Perform fade in/out
    //
    if ( m_bDoFade )
    {
        float delta = gpGlobals->frametime * (1.0f / zm_cl_flashlight_expensive_fadetime.GetFloat());

        if ( m_bPreferExpensive ) // Fade in
        {
            m_flFadeAlpha += delta;
            if ( m_flFadeAlpha >= 1.0f )
            {
                m_flFadeAlpha = 1.0f;
                m_bDoFade = false;
            }
        }
        else // Fade out
        {
            m_flFadeAlpha -= delta;
            if ( m_flFadeAlpha <= 0.0f )
            {
                m_flFadeAlpha = 0.0f;
                m_bDoFade = false;
            }
        }

        clr = m_flFadeAlpha;
    }



    FlashlightState_t state;
    state.m_vecLightOrigin = vOrigin;
    BasisToQuaternion( vDir, vRight, vUp, state.m_quatOrientation );
    state.m_fVerticalFOVDegrees = state.m_fHorizontalFOVDegrees = flFov;
    state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
    state.m_fConstantAtten = r_flashlightconstant.GetFloat();
    state.m_fLinearAtten = flLinearAtten;
    state.m_Color[0] = clr;
    state.m_Color[1] = clr;
    state.m_Color[2] = clr;
    state.m_Color[3] = r_flashlightambient.GetFloat();
    state.m_NearZ = GetFlashlightNearZ();
    state.m_FarZ = GetFlashlightFarZ();
    state.m_bEnableShadows = bDrawShadows;
    state.m_flShadowMapResolution = r_flashlightdepthres.GetInt();
    state.m_pSpotlightTexture = m_FlashlightTexture;
    state.m_nSpotlightTextureFrame = 0;
    state.m_flShadowAtten = r_flashlightshadowatten.GetFloat();
    state.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
    state.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();

    if( m_FlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
    {
        m_FlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
    }
    else
    {
        if( !r_flashlightlockposition.GetBool() )
        {
            g_pClientShadowMgr->UpdateFlashlightState( m_FlashlightHandle, state );
        }
    }
    
    g_pClientShadowMgr->UpdateProjectedTexture( m_FlashlightHandle, true );
    

#ifndef NO_TOOLFRAMEWORK
    if ( clienttools->IsInRecordingMode() )
    {
        auto* msg = new KeyValues( "FlashlightState" );
        msg->SetFloat( "time", gpGlobals->curtime );
        msg->SetInt( "entindex", m_pPlayer->entindex() );
        msg->SetInt( "flashlightHandle", m_FlashlightHandle );
        msg->SetPtr( "flashlightState", &state );
        ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
        msg->deleteThis();
    }
#endif
}

//
// Small, relatively cheap dynamic light.
//
void CZMFlashlightEffect::UpdateLightCheap( const Vector& vecPos, const Vector& vecDir )
{
    if ( !UseThirdpersonEffects() )
    {
        return;
    }


    trace_t tr;
    UTIL_TraceLine( vecPos, vecPos + (vecDir * 200), MASK_SOLID, m_pPlayer, COLLISION_GROUP_NONE, &tr );

    if ( tr.fraction != 1.0f )
    {
        dlight_t* l = effects->CL_AllocDlight( m_pPlayer->entindex() );
        l->origin = tr.endpos;
        l->radius = 50; 
        l->color.r = 200;
        l->color.g = 200;
        l->color.b = 200;
        l->die = gpGlobals->curtime + 0.1f;
    }
}

//
// Thirdperson flashlight beam. 
//
void CZMFlashlightEffect::UpdateBeam( const Vector& vecDir )
{
    if ( !UseThirdpersonEffects() )
    {
        return;
    }


    // We have nothing to attach the flashlight on to.
    if ( m_iAttachmentRH <= 0 )
    {
        return;
    }


    trace_t tr;
    Vector vecOrigin;
    QAngle temp;
    

    m_pPlayer->GetAttachment( m_iAttachmentRH, vecOrigin, temp );

    UTIL_TraceLine( vecOrigin, vecOrigin + (vecDir * 200), MASK_SOLID, m_pPlayer, COLLISION_GROUP_NONE, &tr );


    if ( !m_pFlashlightBeam )
    {
        BeamInfo_t beamInfo;
        beamInfo.m_nType = TE_BEAMPOINTS;
        beamInfo.m_vecStart = tr.startpos;
        beamInfo.m_vecEnd = tr.endpos;
        beamInfo.m_pszModelName = "sprites/glow01.vmt";
        beamInfo.m_pszHaloName = "sprites/glow01.vmt";
        beamInfo.m_flHaloScale = 3.0f;
        beamInfo.m_flWidth = 8.0f;
        beamInfo.m_flEndWidth = 35.0f;
        beamInfo.m_flFadeLength = 300.0f;
        beamInfo.m_flAmplitude = 0.0f;
        beamInfo.m_flBrightness = 60.0f;
        beamInfo.m_flSpeed = 0.0f;
        beamInfo.m_nStartFrame = 0.0f;
        beamInfo.m_flFrameRate = 0.0f;
        beamInfo.m_flRed = 255.0f;
        beamInfo.m_flGreen = 255.0f;
        beamInfo.m_flBlue = 255.0f;
        beamInfo.m_nSegments = 8;
        beamInfo.m_bRenderable = true;
        beamInfo.m_flLife = 0.5f;
        beamInfo.m_nFlags = FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
                
        m_pFlashlightBeam = beams->CreateBeamPoints( beamInfo );
    }

    if ( m_pFlashlightBeam )
    {
        BeamInfo_t beamInfo;
        beamInfo.m_vecStart = tr.startpos;
        beamInfo.m_vecEnd = tr.endpos;
        beamInfo.m_flRed = 255.0;
        beamInfo.m_flGreen = 255.0;
        beamInfo.m_flBlue = 255.0;

        beams->UpdateBeamInfo( m_pFlashlightBeam, beamInfo );
    }
}

//
// Entry
//
void CZMFlashlightEffect::UpdateLight()
{
    if ( !m_bIsOn )
    {
        return;
    }


    Assert( m_pPlayer );


    Vector fwd, right, up;
    Vector pos;

    AngleVectors( m_pPlayer->EyeAngles(), &fwd, &right, &up );
    pos = m_pPlayer->EyePosition();


    //
    // ZMRTODO: We still need to allocate the lights per player.
    // For this reason we don't have the dynamic lights on for the other players.
    //

    const bool bUseFancyLight = LocalPlayerWatchingMe() || m_bPreferExpensive || m_bDoFade;
    //const bool bUseLocalEffects = LocalPlayerWatchingMe();
    const bool bInThirdperson = IsInThirdperson();


    if( bUseFancyLight )
    {
        LightOffCheap();
        UpdateLightNew( pos, fwd, right, up, bInThirdperson );
    }
    else
    {
        LightOffNew();
        UpdateLightCheap( pos, fwd );
    }


    if ( bInThirdperson )
    {
        UpdateBeam( fwd );
    }
    else
    {
        BeamOff();
    }
}

void CZMFlashlightEffect::LightOffNew()
{
#ifndef NO_TOOLFRAMEWORK
    if ( clienttools->IsInRecordingMode() )
    {
        KeyValues *msg = new KeyValues( "FlashlightState" );
        msg->SetFloat( "time", gpGlobals->curtime );
        msg->SetInt( "entindex", m_pPlayer->entindex() );
        msg->SetInt( "flashlightHandle", m_FlashlightHandle );
        msg->SetPtr( "flashlightState", nullptr );
        ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
        msg->deleteThis();
    }
#endif

    // Clear out the light
    if( m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
    {
        g_pClientShadowMgr->DestroyFlashlight( m_FlashlightHandle );
        m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
    }
}

void CZMFlashlightEffect::LightOffCheap()
{	
    // Cheap is turned off automatically.
}

void CZMFlashlightEffect::BeamOff()
{
    if ( m_pFlashlightBeam )
    {
        m_pFlashlightBeam->flags = 0;
        m_pFlashlightBeam->die = gpGlobals->curtime - 1.0f;

        m_pFlashlightBeam = nullptr;
    }
}

void CZMFlashlightEffect::LightOff()
{	
    LightOffNew();
    LightOffCheap();
    BeamOff();
}
