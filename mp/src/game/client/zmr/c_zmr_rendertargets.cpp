#include "cbase.h"

#include "baseclientrendertargets.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define SCOPE_RENDER_SIZE			1024

class CZMClientRenderTargets : public CBaseClientRenderTargets
{
public:
    DECLARE_CLASS_GAMEROOT( CZMClientRenderTargets, CBaseClientRenderTargets );


	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig ) OVERRIDE;
	virtual void ShutdownClientRenderTargets() OVERRIDE;

protected:
	ITexture* CreateScopeTexture( IMaterialSystem* pMaterialSystem );

private:
	CTextureReference m_ScopeTexture;
};

void CZMClientRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets( pMaterialSystem, pHardwareConfig );

	m_ScopeTexture.Init( CreateScopeTexture( pMaterialSystem ) );
}

void CZMClientRenderTargets::ShutdownClientRenderTargets()
{
	BaseClass::ShutdownClientRenderTargets();

	m_ScopeTexture.Shutdown();
}

ITexture* CZMClientRenderTargets::CreateScopeTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Scope",
		SCOPE_RENDER_SIZE, SCOPE_RENDER_SIZE, RT_SIZE_FULL_FRAME_BUFFER,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

static CZMClientRenderTargets g_ZMClientRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CZMClientRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_ZMClientRenderTargets );
CZMClientRenderTargets* g_pZMClientRenderTargets = &g_ZMClientRenderTargets;
