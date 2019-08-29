//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Implementation of IMaterialSystem interface which "passes tru" all 
//			function calls to the real interface. Can be used to override
//			IMaterialSystem function calls (combined with engine->Mat_Stub).
//
//=============================================================================//

#ifndef WARS_MATERIALSYSTEM_PASSTHRU_H
#define WARS_MATERIALSYSTEM_PASSTHRU_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialsystem.h"

class CPassThruMaterialSystem : public IMaterialSystem
{
public:
	typedef CPassThruMaterialSystem BaseClass;

	CPassThruMaterialSystem()
	{
		m_pBaseMaterialsPassThru = NULL;
	}
	virtual void InitPassThru( IMaterialSystem *pBaseMaterialsPassThru )
	{
		m_pBaseMaterialsPassThru = pBaseMaterialsPassThru;
	}

public:
	virtual bool Connect( CreateInterfaceFn factory ) { return m_pBaseMaterialsPassThru->Connect( factory ); }
	virtual void Disconnect() { return m_pBaseMaterialsPassThru->Disconnect( ); }
	virtual void *QueryInterface( const char *pInterfaceName ) { return m_pBaseMaterialsPassThru->QueryInterface( pInterfaceName ); }
	virtual InitReturnVal_t Init() { return m_pBaseMaterialsPassThru->Init( ); }
	virtual void Shutdown() { m_pBaseMaterialsPassThru->Shutdown( ); }

	virtual CreateInterfaceFn	Init( char const* pShaderAPIDLL, 
		IMaterialProxyFactory *pMaterialProxyFactory,
		CreateInterfaceFn fileSystemFactory,
		CreateInterfaceFn cvarFactory=NULL ) { return m_pBaseMaterialsPassThru->Init( pShaderAPIDLL, pMaterialProxyFactory, fileSystemFactory, cvarFactory ); }

	virtual void				SetShaderAPI( char const *pShaderAPIDLL ) { m_pBaseMaterialsPassThru->SetShaderAPI( pShaderAPIDLL ); }

	virtual void				SetAdapter( int nAdapter, int nFlags ) { m_pBaseMaterialsPassThru->SetAdapter( nAdapter, nFlags ); }

	virtual void				ModInit() { m_pBaseMaterialsPassThru->ModInit(); }
	virtual void				ModShutdown() { m_pBaseMaterialsPassThru->ModShutdown(); }

	virtual void				SetThreadMode( MaterialThreadMode_t mode, int nServiceThread = -1 ) { m_pBaseMaterialsPassThru->SetThreadMode( mode, nServiceThread ); }
	virtual MaterialThreadMode_t GetThreadMode() { return m_pBaseMaterialsPassThru->GetThreadMode(); }
	virtual void				ExecuteQueued() { m_pBaseMaterialsPassThru->ExecuteQueued(); }

	virtual IMaterialSystemHardwareConfig *GetHardwareConfig( const char *pVersion, int *returnCode ) { return m_pBaseMaterialsPassThru->GetHardwareConfig( pVersion, returnCode ); }

	virtual bool				UpdateConfig( bool bForceUpdate ) { return m_pBaseMaterialsPassThru->UpdateConfig( bForceUpdate ); }

	virtual bool				OverrideConfig( const MaterialSystem_Config_t &config, bool bForceUpdate ) { return m_pBaseMaterialsPassThru->OverrideConfig( config, bForceUpdate ); }

	virtual const MaterialSystem_Config_t &GetCurrentConfigForVideoCard() const { return m_pBaseMaterialsPassThru->GetCurrentConfigForVideoCard(); }

	virtual bool				GetRecommendedConfigurationInfo( int nDXLevel, KeyValues * pKeyValues ) { return m_pBaseMaterialsPassThru->GetRecommendedConfigurationInfo( nDXLevel, pKeyValues ); }

	virtual int					GetDisplayAdapterCount() const { return m_pBaseMaterialsPassThru->GetDisplayAdapterCount(); }

	virtual int					GetCurrentAdapter() const { return m_pBaseMaterialsPassThru->GetCurrentAdapter(); }

	virtual void				GetDisplayAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const { m_pBaseMaterialsPassThru->GetDisplayAdapterInfo( adapter, info ); }

	virtual int					GetModeCount( int adapter ) const { return m_pBaseMaterialsPassThru->GetModeCount( adapter ); }

	virtual void				GetModeInfo( int adapter, int mode, MaterialVideoMode_t& info ) const { m_pBaseMaterialsPassThru->GetModeInfo( adapter, mode, info ); }

	virtual void				AddModeChangeCallBack( ModeChangeCallbackFunc_t func ) { m_pBaseMaterialsPassThru->AddModeChangeCallBack( func ); }

	virtual void				GetDisplayMode( MaterialVideoMode_t& mode ) const { m_pBaseMaterialsPassThru->GetDisplayMode( mode ); }

	virtual bool				SetMode( void* hwnd, const MaterialSystem_Config_t &config ) { return m_pBaseMaterialsPassThru->SetMode( hwnd, config ); }

	virtual bool				SupportsMSAAMode( int nMSAAMode ) { return m_pBaseMaterialsPassThru->SupportsMSAAMode( nMSAAMode ); }

	virtual const MaterialSystemHardwareIdentifier_t &GetVideoCardIdentifier( void ) const { return m_pBaseMaterialsPassThru->GetVideoCardIdentifier( ); }

	virtual void				SpewDriverInfo() const { m_pBaseMaterialsPassThru->SpewDriverInfo( ); }

	virtual void				GetBackBufferDimensions( int &width, int &height) const { m_pBaseMaterialsPassThru->GetBackBufferDimensions( width, height ); }
	virtual ImageFormat			GetBackBufferFormat() const { return m_pBaseMaterialsPassThru->GetBackBufferFormat( ); }

	virtual bool				SupportsHDRMode( HDRType_t nHDRModede ) { return m_pBaseMaterialsPassThru->SupportsHDRMode( nHDRModede ); }

	virtual bool				AddView( void* hwnd ) { return m_pBaseMaterialsPassThru->AddView( hwnd ); }
	virtual void				RemoveView( void* hwnd ) { m_pBaseMaterialsPassThru->RemoveView( hwnd ); }

	virtual void				SetView( void* hwnd ) { m_pBaseMaterialsPassThru->SetView( hwnd ); }

	virtual void				BeginFrame( float frameTime ) { m_pBaseMaterialsPassThru->BeginFrame( frameTime ); }
	virtual void				EndFrame( ) { m_pBaseMaterialsPassThru->EndFrame( ); }
	virtual void				Flush( bool flushHardware = false ) { m_pBaseMaterialsPassThru->Flush( flushHardware ); }

	virtual void				SwapBuffers( ) { m_pBaseMaterialsPassThru->SwapBuffers( ); }

	virtual void				EvictManagedResources() { m_pBaseMaterialsPassThru->EvictManagedResources( ); }

	virtual void				ReleaseResources(void) { m_pBaseMaterialsPassThru->ReleaseResources( ); }
	virtual void				ReacquireResources(void ) { m_pBaseMaterialsPassThru->ReacquireResources( ); }

	virtual void				AddReleaseFunc( MaterialBufferReleaseFunc_t func ) { m_pBaseMaterialsPassThru->AddReleaseFunc( func ); }
	virtual void				RemoveReleaseFunc( MaterialBufferReleaseFunc_t func ) { m_pBaseMaterialsPassThru->RemoveReleaseFunc( func ); }

	virtual void				AddRestoreFunc( MaterialBufferRestoreFunc_t func ) { m_pBaseMaterialsPassThru->AddRestoreFunc( func ); }
	virtual void				RemoveRestoreFunc( MaterialBufferRestoreFunc_t func ) { m_pBaseMaterialsPassThru->RemoveRestoreFunc( func ); }
	
	virtual void				ResetTempHWMemory( bool bExitingLevel = false ) { m_pBaseMaterialsPassThru->ResetTempHWMemory( bExitingLevel ); }

	virtual void				HandleDeviceLost() { m_pBaseMaterialsPassThru->HandleDeviceLost( ); }

	virtual int					ShaderCount() const { return m_pBaseMaterialsPassThru->ShaderCount( ); }
	virtual int					GetShaders( int nFirstShader, int nMaxCount, IShader **ppShaderList ) const { return m_pBaseMaterialsPassThru->GetShaders( nFirstShader, nMaxCount, ppShaderList ); }

	virtual int					ShaderFlagCount() const { return m_pBaseMaterialsPassThru->ShaderFlagCount(); }
	virtual const char *		ShaderFlagName( int nIndex ) const { return m_pBaseMaterialsPassThru->ShaderFlagName( nIndex ); }

	virtual void				GetShaderFallback( const char *pShaderName, char *pFallbackShader, int nFallbackLength ) { m_pBaseMaterialsPassThru->GetShaderFallback( pShaderName, pFallbackShader, nFallbackLength ); }

	virtual IMaterialProxyFactory *GetMaterialProxyFactory() { return m_pBaseMaterialsPassThru->GetMaterialProxyFactory( ); }

	virtual void				SetMaterialProxyFactory( IMaterialProxyFactory* pFactory ) { m_pBaseMaterialsPassThru->SetMaterialProxyFactory( pFactory ); }

	virtual void				EnableEditorMaterials() { m_pBaseMaterialsPassThru->EnableEditorMaterials( ); }

	virtual void				SetInStubMode( bool bInStubMode ) { m_pBaseMaterialsPassThru->SetInStubMode( bInStubMode ); }

	virtual void				DebugPrintUsedMaterials( const char *pSearchSubString, bool bVerbose ) { m_pBaseMaterialsPassThru->DebugPrintUsedMaterials( pSearchSubString, bVerbose ); }
	virtual void				DebugPrintUsedTextures( void ) { m_pBaseMaterialsPassThru->DebugPrintUsedTextures( ); }

	virtual void				ToggleSuppressMaterial( char const* pMaterialName ) { m_pBaseMaterialsPassThru->ToggleSuppressMaterial( pMaterialName ); }
	virtual void				ToggleDebugMaterial( char const* pMaterialName )  { m_pBaseMaterialsPassThru->ToggleDebugMaterial( pMaterialName ); }

	virtual bool				UsingFastClipping( void ) { return m_pBaseMaterialsPassThru->UsingFastClipping( ); }

	virtual int					StencilBufferBits( void ) { return m_pBaseMaterialsPassThru->StencilBufferBits( ); }

	virtual void				UncacheAllMaterials( ) { m_pBaseMaterialsPassThru->UncacheAllMaterials( ); }

	virtual void				UncacheUnusedMaterials( bool bRecomputeStateSnapshots = false ) { m_pBaseMaterialsPassThru->UncacheUnusedMaterials( bRecomputeStateSnapshots ); }


	virtual void				CacheUsedMaterials( ) { m_pBaseMaterialsPassThru->CacheUsedMaterials( ); }

	virtual void				ReloadTextures( ) { m_pBaseMaterialsPassThru->ReloadTextures( ); }

	virtual void				ReloadMaterials( const char *pSubString = NULL ) { m_pBaseMaterialsPassThru->ReloadMaterials( pSubString ); }

	virtual IMaterial *			CreateMaterial( const char *pMaterialName, KeyValues *pVMTKeyValues ) { return m_pBaseMaterialsPassThru->CreateMaterial( pMaterialName, pVMTKeyValues ); }

	virtual IMaterial *			FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL ) 
				{ return m_pBaseMaterialsPassThru->FindMaterial( pMaterialName, pTextureGroupName, complain, pComplainPrefix ); }

	virtual MaterialHandle_t	FirstMaterial() const { return m_pBaseMaterialsPassThru->FirstMaterial( ); }

	virtual MaterialHandle_t	NextMaterial( MaterialHandle_t h ) const { return m_pBaseMaterialsPassThru->NextMaterial( h ); }

	virtual MaterialHandle_t	InvalidMaterial() const { return m_pBaseMaterialsPassThru->InvalidMaterial( ); }

	virtual IMaterial*			GetMaterial( MaterialHandle_t h ) const { return m_pBaseMaterialsPassThru->GetMaterial( h ); }

	virtual int					GetNumMaterials( ) const { return m_pBaseMaterialsPassThru->GetNumMaterials( ); }

	virtual ITexture *			FindTexture( char const* pTextureName, const char *pTextureGroupName, bool complain = true ) { return m_pBaseMaterialsPassThru->FindTexture( pTextureName, pTextureGroupName, complain ); }

	virtual bool				IsTextureLoaded( char const* pTextureName ) const { return m_pBaseMaterialsPassThru->IsTextureLoaded( pTextureName ); }

	virtual ITexture *			CreateProceduralTexture( const char	*pTextureName, 
		const char *pTextureGroupName, 
		int w, 
		int h, 
		ImageFormat fmt, 
		int nFlags ) { return m_pBaseMaterialsPassThru->CreateProceduralTexture( pTextureName, pTextureGroupName, w, h, fmt, nFlags ); }

	virtual void				BeginRenderTargetAllocation() { return m_pBaseMaterialsPassThru->BeginRenderTargetAllocation(); }
	virtual void				EndRenderTargetAllocation() { return m_pBaseMaterialsPassThru->EndRenderTargetAllocation(); }

	virtual ITexture *			CreateRenderTargetTexture( int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat	format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED ) { return m_pBaseMaterialsPassThru->CreateRenderTargetTexture(w, h, sizeMode, format, depth ); }

	virtual ITexture *			CreateNamedRenderTargetTextureEx(  const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0 ) { return m_pBaseMaterialsPassThru->CreateNamedRenderTargetTextureEx(pRTName, w, h, sizeMode, format, depth, textureFlags, renderTargetFlags ); }

	virtual ITexture *			CreateNamedRenderTargetTexture( const char *pRTName, 
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		bool bClampTexCoords = true, 
		bool bAutoMipMap = false ) { return m_pBaseMaterialsPassThru->CreateNamedRenderTargetTexture(pRTName, w, h, sizeMode, format, depth, bClampTexCoords, bAutoMipMap ); }

	virtual ITexture *			CreateNamedRenderTargetTextureEx2( const char *pRTName,				// Pass in NULL here for an unnamed render target.
		int w, 
		int h, 
		RenderTargetSizeMode_t sizeMode,	// Controls how size is generated (and regenerated on video mode change).
		ImageFormat format, 
		MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, 
		unsigned int textureFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		unsigned int renderTargetFlags = 0 ) { return m_pBaseMaterialsPassThru->CreateNamedRenderTargetTextureEx2(pRTName, w, h, sizeMode, format, depth, textureFlags, renderTargetFlags ); }

	virtual void				BeginLightmapAllocation( ) { m_pBaseMaterialsPassThru->BeginLightmapAllocation(); }
	virtual void				EndLightmapAllocation( ) { m_pBaseMaterialsPassThru->EndLightmapAllocation(); }

	virtual int 				AllocateLightmap( int width, int height, 
		int offsetIntoLightmapPage[2],
		IMaterial *pMaterial ) { return m_pBaseMaterialsPassThru->AllocateLightmap( width, height, offsetIntoLightmapPage, pMaterial ); }
	virtual int					AllocateWhiteLightmap( IMaterial *pMaterial ) { return m_pBaseMaterialsPassThru->AllocateWhiteLightmap( pMaterial ); }

	virtual void				UpdateLightmap( int lightmapPageID, int lightmapSize[2],
		int offsetIntoLightmapPage[2], 
		float *pFloatImage, float *pFloatImageBump1,
		float *pFloatImageBump2, float *pFloatImageBump3 ) { m_pBaseMaterialsPassThru->UpdateLightmap( lightmapPageID, lightmapSize, offsetIntoLightmapPage, pFloatImage, pFloatImageBump1, pFloatImageBump2, pFloatImageBump3 ); }

	virtual int					GetNumSortIDs( ) { return m_pBaseMaterialsPassThru->GetNumSortIDs( ); }
	virtual void				GetSortInfo( MaterialSystem_SortInfo_t *sortInfoArray ) { m_pBaseMaterialsPassThru->GetSortInfo( sortInfoArray ); }

	virtual void				GetLightmapPageSize( int lightmap, int *width, int *height ) const { m_pBaseMaterialsPassThru->GetLightmapPageSize( lightmap, width, height ); }

	virtual void				ResetMaterialLightmapPageInfo() { m_pBaseMaterialsPassThru->ResetMaterialLightmapPageInfo(); }



	virtual void				ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil = false ) { m_pBaseMaterialsPassThru->ClearBuffers( bClearColor, bClearDepth, bClearStencil ); }

	virtual IMatRenderContext *	GetRenderContext() { return m_pBaseMaterialsPassThru->GetRenderContext(); }

	virtual void				BeginUpdateLightmaps( void ) { m_pBaseMaterialsPassThru->BeginUpdateLightmaps(); }
	virtual void				EndUpdateLightmaps( void ) { m_pBaseMaterialsPassThru->EndUpdateLightmaps(); }

	virtual MaterialLock_t		Lock() { return m_pBaseMaterialsPassThru->Lock(); }
	virtual void				Unlock( MaterialLock_t l ) { return m_pBaseMaterialsPassThru->Unlock( l ); }

	virtual IMatRenderContext *CreateRenderContext( MaterialContextType_t type ) { return m_pBaseMaterialsPassThru->CreateRenderContext( type ); }

	virtual IMatRenderContext *SetRenderContext( IMatRenderContext *c )  { return m_pBaseMaterialsPassThru->SetRenderContext( c ); }

	virtual bool				SupportsCSAAMode( int nNumSamples, int nQualityLevel ) { return m_pBaseMaterialsPassThru->SupportsCSAAMode( nNumSamples, nQualityLevel ); }

	virtual void				RemoveModeChangeCallBack( ModeChangeCallbackFunc_t func ) { m_pBaseMaterialsPassThru->RemoveModeChangeCallBack( func ); }

	virtual IMaterial *			FindProceduralMaterial( const char *pMaterialName, const char *pTextureGroupName, KeyValues *pVMTKeyValues ) 
		{ return m_pBaseMaterialsPassThru->FindProceduralMaterial( pMaterialName, pTextureGroupName, pVMTKeyValues ); }

	virtual void				AddTextureAlias( const char *pAlias, const char *pRealName ) { m_pBaseMaterialsPassThru->AddTextureAlias( pAlias, pRealName ); }
	virtual void				RemoveTextureAlias( const char *pAlias ) { m_pBaseMaterialsPassThru->RemoveTextureAlias( pAlias ); }

	virtual int					AllocateDynamicLightmap( int lightmapSize[2], int *pOutOffsetIntoPage, int frameID ) 
		{ return m_pBaseMaterialsPassThru->AllocateDynamicLightmap( lightmapSize, pOutOffsetIntoPage, frameID ); }

	virtual void				SetExcludedTextures( const char *pScriptName ) { m_pBaseMaterialsPassThru->SetExcludedTextures( pScriptName ); }
	virtual void				UpdateExcludedTextures( void ) { m_pBaseMaterialsPassThru->UpdateExcludedTextures( ); }

	virtual bool				IsInFrame( ) const { return m_pBaseMaterialsPassThru->IsInFrame(); }

	virtual void				CompactMemory() { m_pBaseMaterialsPassThru->CompactMemory(); }

	virtual void				ReloadFilesInList( IFileList *pFilesToReload ) { m_pBaseMaterialsPassThru->ReloadFilesInList( pFilesToReload ); }

	virtual	bool				AllowThreading( bool bAllow, int nServiceThread ) { return m_pBaseMaterialsPassThru->AllowThreading( bAllow, nServiceThread ); }
	
	virtual bool				IsRenderThreadSafe() { return m_pBaseMaterialsPassThru->IsRenderThreadSafe(); }
	
	virtual void				GetDXLevelDefaults( uint &max_dxlevel, uint &recomended ) { m_pBaseMaterialsPassThru->GetDXLevelDefaults( max_dxlevel, recomended ); }

	virtual bool				IsMaterialLoaded( const char* name ) { return m_pBaseMaterialsPassThru->IsMaterialLoaded( name ); }

	virtual void				SetAsyncTextureLoadCache( void* data ) { m_pBaseMaterialsPassThru->SetAsyncTextureLoadCache( data ); }

	virtual ITexture*			FindTexture( const char* name, const char* group, bool complain, int flags ) { return m_pBaseMaterialsPassThru->FindTexture( name, group, complain, flags ); }

	virtual bool				SupportsShadowDepthTextures() { return m_pBaseMaterialsPassThru->SupportsShadowDepthTextures(); }

	virtual ImageFormat			GetShadowDepthTextureFormat() { return m_pBaseMaterialsPassThru->GetShadowDepthTextureFormat(); }

	virtual bool				SupportsFetch4() { return m_pBaseMaterialsPassThru->SupportsFetch4(); }

	virtual ImageFormat			GetNullTextureFormat() { return m_pBaseMaterialsPassThru->GetNullTextureFormat(); }

	virtual IMaterial*			FindMaterialEx( char const* pMaterialName, const char *pTextureGroupName, int nContext, bool complain = true, const char *pComplainPrefix = NULL )
	{
		return m_pBaseMaterialsPassThru->FindMaterialEx( pMaterialName, pTextureGroupName, nContext, complain, pComplainPrefix );
	}

#ifdef DX_TO_GL_ABSTRACTION
	virtual void				DoStartupShaderPreloading() { m_pBaseMaterialsPassThru->DoStartupShaderPreloading(); }
#endif	

	virtual void				SetRenderTargetFrameBufferSizeOverrides( int nWidth, int nHeight ) { m_pBaseMaterialsPassThru->SetRenderTargetFrameBufferSizeOverrides( nWidth, nHeight ); }

	virtual void				GetRenderTargetFrameBufferDimensions( int & nWidth, int & nHeight ) { m_pBaseMaterialsPassThru->GetRenderTargetFrameBufferDimensions( nWidth, nHeight ); }

	virtual char*				GetDisplayDeviceName() const { return m_pBaseMaterialsPassThru->GetDisplayDeviceName(); }

	virtual ITexture*			CreateTextureFromBits( int w, int h, int mips, ImageFormat fmt, int srcBufferSize, byte* srcBits )
	{
		return m_pBaseMaterialsPassThru->CreateTextureFromBits( w, h, mips, fmt, srcBufferSize, srcBits );
	}

	virtual void				OverrideRenderTargetAllocation( bool rtAlloc ) { m_pBaseMaterialsPassThru->OverrideRenderTargetAllocation( rtAlloc ); }

	virtual ITextureCompositor*	NewTextureCompositor( int w, int h, const char* pCompositeName, int nTeamNum, uint64 randomSeed, KeyValues* stageDesc, uint32 texCompositeCreateFlags = 0 )
	{
		return m_pBaseMaterialsPassThru->NewTextureCompositor( w, h, pCompositeName, nTeamNum, randomSeed, stageDesc, texCompositeCreateFlags );
	}

	virtual void				AsyncFindTexture( const char* pFilename, const char *pTextureGroupName, IAsyncTextureOperationReceiver* pRecipient, void* pExtraArgs, bool bComplain = true, int nAdditionalCreationFlags = 0 )
	{
		m_pBaseMaterialsPassThru->AsyncFindTexture( pFilename, pTextureGroupName, pRecipient, pExtraArgs, bComplain, nAdditionalCreationFlags );
	}

	virtual ITexture*			CreateNamedTextureFromBitsEx( const char* pName, const char *pTextureGroupName, int w, int h, int mips, ImageFormat fmt, int srcBufferSize, byte* srcBits, int nFlags )
	{
		return m_pBaseMaterialsPassThru->CreateNamedTextureFromBitsEx( pName, pTextureGroupName, w, h, mips, fmt, srcBufferSize, srcBits, nFlags );
	}

protected:
	IMaterialSystem *m_pBaseMaterialsPassThru;
};

#endif // WARS_MATERIALSYSTEM_PASSTHRU_H