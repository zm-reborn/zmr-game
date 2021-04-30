#include "cbase.h"

#include "view.h"
#include "viewrender.h"

#include <tier0/vprof.h>
#include <materialsystem/itexture.h>

#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ITexture* GetScopeTexture()
{ 
    static CTextureReference s_pScopeTexture;
    if ( !s_pScopeTexture.IsValid() )
    {
        s_pScopeTexture.Init( materials->FindTexture( "_rt_Scope", TEXTURE_GROUP_RENDER_TARGET ) );
        Assert( !IsErrorTexture( s_pScopeTexture ) );
    }

    return s_pScopeTexture;
}

IMaterial* GetScopeRefractionMaterial()
{
	static IMaterial* s_pScopeRefraction = nullptr;
	if ( !s_pScopeRefraction )
	{
		s_pScopeRefraction = materials->FindMaterial( "models/weapons/scope/scope_refract", TEXTURE_GROUP_CLIENT_EFFECTS, false );

		if ( s_pScopeRefraction )
		{
			s_pScopeRefraction->IncrementReferenceCount();
		}
	}

	return s_pScopeRefraction;
}

IMaterial* GetScopeReticleMaterial()
{
	static IMaterial* s_pScopeReticle = nullptr;
	if ( !s_pScopeReticle )
	{
		s_pScopeReticle = materials->FindMaterial( "models/weapons/scope/reticle", TEXTURE_GROUP_CLIENT_EFFECTS, false );

		if ( s_pScopeReticle )
		{
			s_pScopeReticle->IncrementReferenceCount();
		}
	}

	return s_pScopeReticle;
}

IMaterial* GetScopeBordersMaterial()
{
	static IMaterial* s_pScopeBorders = nullptr;
	if ( !s_pScopeBorders )
	{
		s_pScopeBorders = materials->FindMaterial( "models/weapons/scope/scope_end_mask", TEXTURE_GROUP_CLIENT_EFFECTS, false );

		if ( s_pScopeBorders )
		{
			s_pScopeBorders->IncrementReferenceCount();
		}
	}

	return s_pScopeBorders;
}

static float s_bRenderedEmptyScopeAlpha = -1.0f;

void RenderEmptyScope( ITexture* pScopeTexture, float alpha = 0.0f )
{
	// Don't need to rerender with the same alpha.
	if ( alpha == s_bRenderedEmptyScopeAlpha ) return;


	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushRenderTargetAndViewport( pScopeTexture );
	
	{
		pRenderContext->OverrideAlphaWriteEnable( true, true );

		pRenderContext->ClearColor4ub( 0, 0, 0, 255 * alpha );
		pRenderContext->ClearBuffers( true, false );

		pRenderContext->OverrideAlphaWriteEnable( false, false );
	}
	
	pRenderContext->PopRenderTargetAndViewport();


	s_bRenderedEmptyScopeAlpha = alpha;
}

//
// Renders the scope
//
void CViewRender::DrawScope( const CViewSetup &cameraView )
{
	auto* pScopeTexture = GetScopeTexture();
	if ( !pScopeTexture ) return;


	// Always render an empty scope
	// In case something is trying to draw one.
	auto* pLocalPlayer = C_ZMPlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
	{
		RenderEmptyScope( pScopeTexture );
		return;
	}

	auto* pVM = pLocalPlayer->GetViewModel();
	auto* pWeapon = pLocalPlayer->GetActiveZMWeapon();
	if ( !pVM || !pWeapon || !pVM->ShouldRenderScope() )
	{
		RenderEmptyScope( pScopeTexture );
		return;
	}





	const float flIronsightFrac = pVM->GetIronSightFraction();
	const float flScopeAlphaFracStart = 0.5f;

	// Fade in the scope when the eye gets near it.
	float flScopeAlpha;

	if ( flIronsightFrac < flScopeAlphaFracStart )
	{
		flScopeAlpha = 0.0f;
	}
	else
	{
		flScopeAlpha = (flIronsightFrac - flScopeAlphaFracStart) / (1.0f - flScopeAlphaFracStart);
	}


	// Nothing to render!
	if ( flScopeAlpha <= 0.0f )
	{
		RenderEmptyScope( pScopeTexture );
		return;
	}

	
	Vector vecScopePos;
	QAngle angScope;
	pVM->GetScopeEndPosition( vecScopePos, angScope );
	
	Vector fwd;
	AngleVectors( angScope, &fwd );


	// Check to see if there's something in front of
	// the camera to not render things through walls.
	trace_t tr;
	CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NONE );
	UTIL_TraceLine( MainViewOrigin(), vecScopePos + fwd * 10.0f, MASK_SOLID, &filter, &tr );

	if ( tr.fraction != 1.0f )
	{
		RenderEmptyScope( pScopeTexture, flScopeAlpha );
		return;
	}


	//
	// Render the scope view!
	//
	s_bRenderedEmptyScopeAlpha = -1.0f;


	VPROF_INCREMENT_COUNTER( "scopes rendered", 1 );

	
	CMatRenderContextPtr pRenderContext( materials );
	
	// Setup view.
	CViewSetup scopeView = cameraView;

	scopeView.origin = vecScopePos;
	scopeView.angles = angScope;
	scopeView.x = 0;
	scopeView.y = 0;
	scopeView.width = pScopeTexture->GetActualWidth();
	scopeView.height = pScopeTexture->GetActualHeight();
	scopeView.m_bOrtho = false;
	scopeView.m_flAspectRatio = 1.0f;
	scopeView.m_bViewToProjectionOverride = false;
	scopeView.fov = pLocalPlayer->GetFOV() * pWeapon->GetScopeFOVModifier() * 0.5f; // This 0.5 is a supa hack.


	
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 * flScopeAlpha );

	static Frustum dummyFrustum;
	render->Push3DView( scopeView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH, pScopeTexture, dummyFrustum );


	// Draw the scope scene
	pRenderContext->OverrideAlphaWriteEnable( true, false );

	ViewDrawScene( false, SKYBOX_2DSKYBOX_VISIBLE, scopeView, 0, VIEW_MONITOR );

	pRenderContext->OverrideAlphaWriteEnable( false, true );

	// The distortion effect
	auto* pRefractionOverlay = GetScopeRefractionMaterial();
	if ( pRefractionOverlay && !IsErrorMaterial( pRefractionOverlay ) )
	{
		pRenderContext->DrawScreenSpaceRectangle(
			pRefractionOverlay,
			0, 0, scopeView.width, scopeView.height,
			0, 0, scopeView.width - 1, scopeView.height - 1,
			scopeView.width, scopeView.height );
	}
	

	// Apply the reticle on top of the world texture.
	// We assume the survivor has corrected the scope parallax :P
	auto* pReticleOverlay = GetScopeReticleMaterial();
	if ( pReticleOverlay && !IsErrorMaterial( pReticleOverlay ) )
	{
		pRenderContext->DrawScreenSpaceRectangle(
			pReticleOverlay,
			0, 0, scopeView.width, scopeView.height,
			0, 0, scopeView.width - 1, scopeView.height - 1,
			scopeView.width, scopeView.height );
	}

	// Apply scope.
	auto* pScopeBorders = GetScopeBordersMaterial();
	if ( pScopeBorders && !IsErrorMaterial( pScopeBorders ) )
	{
		pRenderContext->DrawScreenSpaceRectangle(
			pScopeBorders,
			0, 0, scopeView.width, scopeView.height,
			0, 0, scopeView.width - 1, scopeView.height - 1,
			scopeView.width, scopeView.height );
	}

	// Done
	render->PopView( dummyFrustum );

	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
}
