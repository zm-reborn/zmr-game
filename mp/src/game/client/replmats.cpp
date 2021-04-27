//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "igamesystem.h"
#include "materialsystem_passtru.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static int matCount = 0;
CON_COMMAND(print_num_replaced_mats, "")
{
	ConColorMsg( COLOR_GREEN, "%d replaced materials\n", matCount );
}

bool replMatPossible = false;

//-----------------------------------------------------------------------------
// List of materials that should be replaced
//-----------------------------------------------------------------------------
static const char * const pszShaderReplaceDict[][2] = {
	{ "LightmappedGeneric",		"SDK_LightmappedGeneric" },
};
static const int iNumShaderReplaceDict = ARRAYSIZE( pszShaderReplaceDict );

#include "icommandline.h"

// Copied from cdeferred_manager_client.cpp
static void ShaderReplaceReplMat( const char *szNewShadername, IMaterial *pMat )
{
	const char *pszOldShadername = pMat->GetShaderName();
	const char *pszMatname = pMat->GetName();

	KeyValues *msg = new KeyValues( szNewShadername );

	int nParams = pMat->ShaderParamCount();
	IMaterialVar **pParams = pMat->GetShaderParams();

	char str[ 512 ];

	for ( int i = 0; i < nParams; ++i )
	{
		IMaterialVar *pVar = pParams[ i ];
		const char *pVarName = pVar->GetName();

		if (!V_stricmp("$flags", pVarName) || 
			!V_stricmp("$flags_defined", pVarName) || 
			!V_stricmp("$flags2", pVarName) || 
			!V_stricmp("$flags_defined2", pVarName) )
			continue;

		switch ( pVar->GetType() )
		{
			case MATERIAL_VAR_TYPE_FLOAT:
				msg->SetFloat( pVarName, pVar->GetFloatValue() );
				break;

			case MATERIAL_VAR_TYPE_INT:
				msg->SetInt( pVarName, pVar->GetIntValue() );
				break;

			case MATERIAL_VAR_TYPE_STRING:
				msg->SetString( pVarName, pVar->GetStringValue() );
				break;

			case MATERIAL_VAR_TYPE_FOURCC:
				//Assert( 0 ); // JDTODO
				break;

			case MATERIAL_VAR_TYPE_VECTOR:
			{
				const float *pVal = pVar->GetVecValue();
				int dim = pVar->VectorSize();
				switch ( dim )
				{
				case 1:
					V_sprintf_safe( str, "[%f]", pVal[ 0 ] );
					break;
				case 2:
					V_sprintf_safe( str, "[%f %f]", pVal[ 0 ], pVal[ 1 ] );
					break;
				case 3:
					V_sprintf_safe( str, "[%f %f %f]", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ] );
					break;
				case 4:
					V_sprintf_safe( str, "[%f %f %f %f]", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ], pVal[ 3 ] );
					break;
				default:
					Assert( 0 );
					*str = 0;
				}
				msg->SetString( pVarName, str );
			}
				break;

			case MATERIAL_VAR_TYPE_MATRIX:
			{
				const float *pVal = pVar->GetMatrixValue().Base();
				V_sprintf_safe( str,
					"[%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f]",
					pVal[ 0 ],  pVal[ 1 ],  pVal[ 2 ],  pVal[ 3 ],
					pVal[ 4 ],  pVal[ 5 ],  pVal[ 6 ],  pVal[ 7 ],
					pVal[ 8 ],  pVal[ 9 ],  pVal[ 10 ], pVal[ 11 ],
					pVal[ 12 ], pVal[ 13 ], pVal[ 14 ], pVal[ 15 ] );
				msg->SetString( pVarName, str );
			}
			break;

			case MATERIAL_VAR_TYPE_TEXTURE:
				msg->SetString( pVarName, pVar->GetTextureValue()->GetName() );
				break;

			case MATERIAL_VAR_TYPE_MATERIAL:
				msg->SetString( pVarName, pVar->GetMaterialValue()->GetName() );
				break;
		}
	}

	bool alphaBlending = pMat->IsTranslucent() || pMat->GetMaterialVarFlag( MATERIAL_VAR_TRANSLUCENT );
	bool translucentOverride = pMat->IsAlphaTested() || pMat->GetMaterialVarFlag( MATERIAL_VAR_ALPHATEST );

	bool bDecal = (pszOldShadername != NULL && Q_stristr( pszOldShadername, "decal" ) != NULL) ||
		(pszMatname != NULL && Q_stristr( pszMatname, "decal" ) != NULL) ||
		pMat->GetMaterialVarFlag( MATERIAL_VAR_DECAL );

	if ( bDecal )
		msg->SetInt( "$decal", 1 );
	
	if ( alphaBlending )
		msg->SetInt("$translucent", 1);

	if ( translucentOverride )
		msg->SetInt( "$alphatest", 1 );

	if ( pMat->IsTwoSided() || pMat->GetMaterialVarFlag(MATERIAL_VAR_NOCULL))
		msg->SetInt( "$nocull", 1 );

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_ADDITIVE))
		msg->SetInt("$additive", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_MODEL))
		msg->SetInt("$model", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_NOFOG))
		msg->SetInt("$nofog", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_IGNOREZ))
		msg->SetInt("$ignorez", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_HALFLAMBERT))
		msg->SetInt("$halflambert", 1);

	pMat->SetShaderAndParams(msg);

	pMat->RefreshPreservingMaterialVars();

	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Overrides FindMaterial and replaces the material if the shader name is found
// in "shadernames_tocheck".
// TODO: Also override the reload material functions and update the material, 
//		 otherwise we keep reading from the replacement directory.
//-----------------------------------------------------------------------------
class CReplMaterialSystem : public CPassThruMaterialSystem
{
public:
	IMaterial* FindMaterialEx( char const* pMaterialName, const char *pTextureGroupName, int nContext, bool complain = true, const char *pComplainPrefix = NULL ) OVERRIDE
	{
		return ReplaceMaterialInternal( BaseClass::FindMaterialEx( pMaterialName, pTextureGroupName, nContext, complain, pComplainPrefix ) );
	}

	IMaterial* FindMaterial( char const* pMaterialName, const char *pTextureGroupName, bool complain = true, const char *pComplainPrefix = NULL ) OVERRIDE
	{
		return ReplaceMaterialInternal( BaseClass::FindMaterial( pMaterialName, pTextureGroupName, complain, pComplainPrefix ) );
	}

	IMaterial* FindProceduralMaterial( const char *pMaterialName, const char *pTextureGroupName, KeyValues *pVMTKeyValues )	OVERRIDE
	{
		if (replMatPossible)
		{
			const char* pShaderName = pVMTKeyValues->GetName();
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName)
				{
					pVMTKeyValues->SetName(pszShaderReplaceDict[i][1]);
					matCount++;
					break;
				}
			}
		}

		return BaseClass::FindProceduralMaterial( pMaterialName, pTextureGroupName, pVMTKeyValues );
	}

	IMaterial* CreateMaterial( const char *pMaterialName, KeyValues *pVMTKeyValues ) OVERRIDE
	{
		if (replMatPossible)
		{
			const char* pShaderName = pVMTKeyValues->GetName();
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName)
				{
					pVMTKeyValues->SetName(pszShaderReplaceDict[i][1]);
					matCount++;
					break;
				}
			}
		}

		return BaseClass::CreateMaterial( pMaterialName, pVMTKeyValues );
	}
private:
	IMaterial* ReplaceMaterialInternal( IMaterial* pMat ) const
	{
		if ( !pMat || pMat->IsErrorMaterial() || !replMatPossible)
			return pMat;

		const char *pShaderName = pMat->GetShaderName();
		if ( pShaderName )
		{
			for ( int i = 0; i < iNumShaderReplaceDict; i++ )
			{
				if ( Q_stristr( pShaderName, pszShaderReplaceDict[i][0] ) == pShaderName )
				{
					ShaderReplaceReplMat( pszShaderReplaceDict[i][1], pMat );
					matCount++;
					break;
				}
			}
		}
		return pMat;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ReplacementSystem : public CAutoGameSystem
{
public:
	ReplacementSystem() : m_pOldMaterialSystem(NULL) {}

	virtual bool Init() 
	{ 
		Enable(); 
		return true; 
	}
	virtual void Shutdown() { Disable(); }
	virtual void LevelShutdownPostEntity() { /*matCount = 0;*/ }

	void Enable();
	void Disable();
	bool IsEnabled() const { return m_pOldMaterialSystem != NULL; }

private:
	CReplMaterialSystem m_MatSysPassTru;
	IMaterialSystem *m_pOldMaterialSystem;
};

static ReplacementSystem s_ReplacementSystem;

void ReplacementSystem::Enable()
{
    // ZMRCHANGE: Change when we actually need to replace materials.
	replMatPossible = CommandLine()->CheckParm( "-replacematerials" );

	if( m_pOldMaterialSystem || !replMatPossible )
		return;

	DevMsg("Enabled material replacement system\n");

	// Replace material system
	m_MatSysPassTru.InitPassThru( materials );

	m_pOldMaterialSystem = materials;
	materials = &m_MatSysPassTru;
	g_pMaterialSystem = &m_MatSysPassTru;
	engine->Mat_Stub( &m_MatSysPassTru );
}

void ReplacementSystem::Disable()
{
	if( m_pOldMaterialSystem )
	{
		DevMsg("Disabled material replacement system\n");

		materials = m_pOldMaterialSystem;
		g_pMaterialSystem = m_pOldMaterialSystem;
		engine->Mat_Stub( m_pOldMaterialSystem );
		m_pOldMaterialSystem = NULL;
	}
}

CON_COMMAND_F( toggle_replmat, "Toggles the material replacement system", FCVAR_CHEAT )
{
	if( s_ReplacementSystem.IsEnabled() )
	{
		s_ReplacementSystem.Disable();
	}
	else
	{
		s_ReplacementSystem.Enable();
	}

	materials->UncacheAllMaterials();
	materials->CacheUsedMaterials();
	materials->ReloadMaterials();
}
