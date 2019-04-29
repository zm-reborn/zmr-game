#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "model_types.h"

#include "c_zmr_player.h"
#include "c_zmr_util.h"
#include "c_zmr_zmvision.h"


ConVar zm_cl_zmvision_dlight( "zm_cl_zmvision_dlight", "1", FCVAR_ARCHIVE );

ConVar zm_cl_silhouette_onlyzmvision( "zm_cl_silhouette_onlyzmvision", "1", FCVAR_ARCHIVE, "Are silhouettes rendered only when ZM vision is on?" );
ConVar zm_cl_silhouette_strength( "zm_cl_silhouette_strength", "0.8", FCVAR_ARCHIVE );




CZMVision g_ZMVision;


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
    if ( !IsOn() ) return;

    if ( !zm_cl_zmvision_dlight.GetBool() ) return;


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pPlayer ) return;


    dlight_t* dl = effects->CL_AllocDlight( pPlayer->entindex() );

    if ( !dl ) return;

    
    dl->origin          = pPlayer->EyePosition();

    dl->color.r         = 255;
    dl->color.g         = 100;
    dl->color.b         = 100;
    dl->color.exponent  = 2;

    dl->radius          = 4096.0f;
    dl->decay           = dl->radius;
    dl->die             = gpGlobals->curtime + 1.0f;
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

void CZMVision::SetVisionOff() { m_bIsOn = false; };

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
    g_ZMVision.Toggle();
}
