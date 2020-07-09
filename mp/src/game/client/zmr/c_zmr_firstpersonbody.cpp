#include "cbase.h"
#include "bone_setup.h"

#include <tier0/vprof.h>


#include "c_zmr_firstpersonbody.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


int UTIL_CreateClientModel( const char* pszModel );

// We could render this as something else to lower the field of view
// and to let it render on top of other things.
#define FIRSTPERSONBODY_RENDER_GROUP			RENDER_GROUP_OPAQUE_ENTITY

// ZMRTODO: If we ever care enough, we need to re-do this completely.
// This is not a good way of doing this.
bool C_ZMFirstPersonBody::Initialize( C_BasePlayer* pOwner )
{
    auto* pModel = pOwner->GetModel();
    if ( !pModel )
        return false;
    
    const char* pszModelName = modelinfo->GetModelName( pModel );

    if ( !pszModelName || !*pszModelName )
        return false;

    int modelIndex = UTIL_CreateClientModel( pszModelName );
    if ( modelIndex == -1 )
        return false;

    if ( !InitializeAsClientEntityByIndex( modelIndex, RENDER_GROUP_OPAQUE_ENTITY ) )
    {
        return false;
    }


    AttachToEntity( pOwner );


	SetNextClientThink( CLIENT_THINK_NEVER );

    return true;
}

void C_ZMFirstPersonBody::AttachToEntity( C_BasePlayer* pOwner )
{
    // Disables annoying asserts.
    bool lastvalid = C_BaseEntity::IsAbsQueriesValid();
    C_BaseEntity::SetAbsQueriesValid( true );

    FollowEntity( pOwner, true );

    C_BaseEntity::SetAbsQueriesValid( lastvalid );

    UpdateVisibility();
}

void C_ZMFirstPersonBody::BuildTransformations( CStudioHdr* pHdr, Vector* pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList& boneComputed )
{
	BaseClass::BuildTransformations( pHdr, pos, q, cameraTransform, boneMask, boneComputed );
	
	VPROF_BUDGET( "C_ZMFirstPersonBody::BuildTransformations", VPROF_BUDGETGROUP_CLIENT_ANIMATION );

	if ( !pHdr )
		return;


	matrix3x4_t bonematrix;

	static bool bOkBone[MAXSTUDIOBONES];

	memset( (void*)bOkBone, 0, sizeof( bOkBone ) );

	// TODO: Cache this.

	// Scale all other bones down to 0.
	static const char* okBones[] = {
		"ValveBiped.Bip01_R_Thigh",
		"ValveBiped.Bip01_R_Calf",
		"ValveBiped.Bip01_R_Foot",
		"ValveBiped.Bip01_R_Toe0",
		"ValveBiped.Bip01_L_Thigh",
		"ValveBiped.Bip01_L_Calf",
		"ValveBiped.Bip01_L_Foot",
		"ValveBiped.Bip01_L_Toe0",
		"ValveBiped.Bip01_Pelvis",
		"ValveBiped.Bip01_Spine",
		"ValveBiped.Bip01_Spine1",
		//"ValveBiped.Bip01_Spine2",
		//"ValveBiped.Bip01_Spine4"
	};


	for ( int i = 0; i < ARRAYSIZE( okBones ); i++ )
	{
		int index = Studio_BoneIndexByName( pHdr, okBones[i] );

		if ( index != -1 )
		{
			bOkBone[index] = true;
		}
	}


	// Move the hidden bones to this bone, because the mesh is still slightly visible.
	int iHideBone = Studio_BoneIndexByName( pHdr, okBones[ARRAYSIZE( okBones ) - 1] );

	Vector hideBoneSpot;
	QAngle ang;
	GetBonePosition( iHideBone != -1 ? iHideBone : 0, hideBoneSpot, ang );
	

	for ( int i = 0; i < pHdr->numbones(); i++ ) 
	{
		// Only update bones reference by the bone mask.
		if ( !(pHdr->boneFlags( i ) & boneMask) )
		{
			continue;
		}

		
		if ( !bOkBone[i] )
		{
			auto& transform = GetBoneForWrite( i );

			VectorScale( transform[0], 0.0f, transform[0] );
			VectorScale( transform[1], 0.0f, transform[1] );
			VectorScale( transform[2], 0.0f, transform[2] );
			MatrixSetColumn( hideBoneSpot, 3, transform );
		}
	}
}

//RenderGroup_t C_ZMFirstPersonBody::GetRenderGroup()
//{
//	return FIRSTPERSONBODY_RENDER_GROUP;
//}

const Vector& C_ZMFirstPersonBody::GetRenderOrigin()
{
    auto* pOwner = GetMoveParent();

    return pOwner ? pOwner->GetRenderOrigin() : BaseClass::GetRenderOrigin();
}

const QAngle& C_ZMFirstPersonBody::GetRenderAngles()
{
    auto* pOwner = GetMoveParent();

    return pOwner ? pOwner->GetRenderAngles() : BaseClass::GetRenderAngles();
}
