#include "cbase.h"
#include "model_types.h"
#include "view.h"

#include "c_zmr_player.h"
#include "c_zmr_util.h"
#include "c_zmr_zmvision.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_zmvision_dlight( "zm_cl_zmvision_dlight", "1", FCVAR_ARCHIVE, "Does ZM vision light up the area? No longer a dynamic light." );
ConVar zm_cl_zmvision_color( "zm_cl_zmvision_color", "0.6 0.2 0.2 1", FCVAR_ARCHIVE, "" );
ConVar zm_cl_zmvision_farz( "zm_cl_zmvision_farz", "4096" );

ConVar zm_cl_zmvision_playsound( "zm_cl_zmvision_playsound", "1", FCVAR_ARCHIVE, "Is a sound played when ZM Vision is toggled?" );

ConVar zm_cl_silhouette_onlyzmvision( "zm_cl_silhouette_onlyzmvision", "1", FCVAR_ARCHIVE, "Are silhouettes rendered only when ZM vision is on?" );
ConVar zm_cl_silhouette_strength( "zm_cl_silhouette_strength", "0.8", FCVAR_ARCHIVE );

extern ConVar r_flashlightnear;
extern ConVar r_flashlightfar;
extern ConVar r_flashlightdepthres;
extern ConVar r_flashlightconstant;
extern ConVar r_flashlightlinear;
extern ConVar r_flashlightquadratic;
extern ConVar r_flashlightambient;
extern ConVar r_flashlightshadowatten;
extern ConVar mat_slopescaledepthbias_shadowmap;
extern ConVar mat_depthbias_shadowmap;

void UTIL_ParseFloatColorFromString( const char* str, float clr[], int nColors );


CZMVision g_ZMVision;

CZMVision::CZMVision()
{
    m_Silhouettes.Purge();
    m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
}

void CZMVision::Init()
{
    m_FlashlightTexture.Init( "effects/flashlight_zm", TEXTURE_GROUP_OTHER );
}

void CZMVision::RenderSilhouette()
{
    C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();

    if ( !pLocal ) return;

    if ( !pLocal->IsZM() ) return;

    if ( zm_cl_silhouette_onlyzmvision.GetBool() && !IsOn() )
        return;


    int i;
    int len = m_Silhouettes.Count();


    if ( len < 1 ) return;

    bool bRender = false;
    for ( i = 0; i < len; i++ )
    {
        if ( m_Silhouettes[i]->ShouldDraw() )
        {
            bRender = true;
            break;
        }
    }

    if ( !bRender ) return;


    g_pStudioRender->ForcedMaterialOverride( materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true ) );

    CMatRenderContextPtr pRenderContext( materials );

    pRenderContext->OverrideDepthEnable( true, false );

    pRenderContext->SetStencilEnable( true );

    pRenderContext->SetStencilReferenceValue( 2 );
    pRenderContext->SetStencilWriteMask( 2 );
    pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_ALWAYS );
    pRenderContext->SetStencilPassOperation( STENCILOPERATION_REPLACE );
    pRenderContext->SetStencilFailOperation( STENCILOPERATION_KEEP );
    pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );

    // Draw depthtest-passing pixels to the stencil buffer
    render->SetBlend( 0 );
    pRenderContext->OverrideAlphaWriteEnable( true, false );
    pRenderContext->OverrideColorWriteEnable( true, false );

    for ( i = 0; i < len; i++ )
    {
        if ( m_Silhouettes[i]->ShouldDraw() )
            m_Silhouettes[i]->DrawModel( STUDIO_RENDER );
    }


    pRenderContext->OverrideAlphaWriteEnable( false, true );
    pRenderContext->OverrideColorWriteEnable( false, true );

    pRenderContext->OverrideDepthEnable( false, false );

    
    pRenderContext->SetStencilReferenceValue( 3 );
    pRenderContext->SetStencilTestMask( 2 );
    pRenderContext->SetStencilWriteMask( 1 );
    pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_NOTEQUAL );
    pRenderContext->SetStencilPassOperation( STENCILOPERATION_REPLACE );
    pRenderContext->SetStencilZFailOperation( STENCILOPERATION_REPLACE );
    pRenderContext->SetStencilFailOperation( STENCILOPERATION_KEEP );


    // Draw color+alpha, stenciling out pixels from the first pass
    const Vector vGlowColor = Vector( 1.0f, 0.0f, 0.0f ) * zm_cl_silhouette_strength.GetFloat();

    render->SetBlend( 1 );
    render->SetColorModulation( vGlowColor.Base() );

    for ( i = 0; i < len; i++ )
    {
        if ( m_Silhouettes[i]->ShouldDraw() )
            m_Silhouettes[i]->DrawModel( STUDIO_RENDER );
    }

    g_pStudioRender->ForcedMaterialOverride( nullptr );
    pRenderContext->SetStencilEnable( false );
}

void CZMVision::UpdateLight()
{
    if ( !IsOn() || !zm_cl_zmvision_dlight.GetBool() )
    {
        DestroyFlashlight();
        return;
    }

    auto* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer )
    {
        DestroyFlashlight();
        return;
    }

    auto pos = MainViewOrigin();
    auto fwd = MainViewForward();
    auto up = MainViewUp();
    auto right = MainViewRight();

    float fov = pPlayer->GetFOV();

    auto aspect_ratio = engine->GetScreenAspectRatio();
    if ( aspect_ratio <= 0.0f )
    {
        aspect_ratio = 1.0f;
    }

    aspect_ratio *= 0.75f; // ??? Taken from ScreenToWorld. It works...

    float colors[4];
    UTIL_ParseFloatColorFromString( zm_cl_zmvision_color.GetString(), colors, 4 );

    FlashlightState_t state;
    state.m_vecLightOrigin = pos;
    BasisToQuaternion( fwd, right, up, state.m_quatOrientation );
    state.m_fHorizontalFOVDegrees = fov * aspect_ratio;
    state.m_fVerticalFOVDegrees = fov;
    state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
    state.m_fConstantAtten = r_flashlightconstant.GetFloat();
    state.m_fLinearAtten = r_flashlightlinear.GetFloat();
    state.m_Color[0] = colors[0];
    state.m_Color[1] = colors[1];
    state.m_Color[2] = colors[2];
    state.m_Color[3] = colors[3];
    state.m_NearZ = r_flashlightnear.GetFloat();
    state.m_FarZ = zm_cl_zmvision_farz.GetFloat();
    state.m_bEnableShadows = false;
    state.m_flShadowMapResolution = r_flashlightdepthres.GetInt();
    state.m_pSpotlightTexture = m_FlashlightTexture;
    state.m_nSpotlightTextureFrame = 0;
    state.m_flShadowAtten = r_flashlightshadowatten.GetFloat();
    state.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
    state.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();

    if ( m_FlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
    {
        m_FlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
    }
    else
    {
        g_pClientShadowMgr->UpdateFlashlightState( m_FlashlightHandle, state );
    }
    
    g_pClientShadowMgr->UpdateProjectedTexture( m_FlashlightHandle, true );
}

void CZMVision::TurnOff()
{
    SetVision( false );
}

void CZMVision::TurnOn()
{
    return SetVision( true );
}

void CZMVision::Toggle()
{
    SetVision( !IsOn() );
}

void CZMVision::SetVisionOff()
{
    m_bIsOn = false;
    DestroyFlashlight();
}

void CZMVision::DestroyFlashlight()
{
    if ( m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
    {
        g_pClientShadowMgr->DestroyFlashlight( m_FlashlightHandle );
        m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
    }
}

void CZMVision::SetVision( bool bEnable )
{
    C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();

    if ( !pLocal || !pLocal->IsZM() )
    {
        SetVisionOff();
        return;
    }


    if ( bEnable != m_bIsOn )
    {
        ZMClientUtil::PrintNotify( "#ZMToggleVision", ZMCHATNOTIFY_ZM );
    }

    m_bIsOn = bEnable;
    
    if ( !bEnable )
    {
        DestroyFlashlight();
    }
}

void CZMVision::AddSilhouette( C_BaseEntity* pEnt )
{
    if ( m_Silhouettes.Find( pEnt ) == -1 )
    {
        m_Silhouettes.AddToTail( pEnt );
    }
}

void CZMVision::RemoveSilhouette( C_BaseEntity* pEnt )
{
    int i = m_Silhouettes.Find( pEnt );
    if ( i != -1 )
    {
        m_Silhouettes.Remove( i );
    }
}

CON_COMMAND( zm_vision, "Toggles ZM vision." )
{
    bool bPrevState = g_ZMVision.IsOn();

    g_ZMVision.Toggle();


    auto* pLocalPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( bPrevState != g_ZMVision.IsOn() && pLocalPlayer && zm_cl_zmvision_playsound.GetBool() )
    {
        if ( g_ZMVision.IsOn() )
        {
            pLocalPlayer->EmitSound( "ZMPlayer.ZMVisionOff" );
        }
        else
        {
            pLocalPlayer->EmitSound( "ZMPlayer.ZMVisionOn" );
        }
    }
}
