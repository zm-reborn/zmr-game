#include "cbase.h"

#include "c_zmr_hat.h"


// ZMRTODO: Put this somewhere nice.
static int UTIL_CreateClientModel( const char* pszModel )
{
    int index;
    index = modelinfo->GetModelIndex( pszModel );

    if ( index == -1 )
    {
        // No model found, register our own.
        index = modelinfo->RegisterDynamicModel( pszModel, true );
    }
    
    return index;
}


// ZMRTODO: If we ever care enough, we need to re-do this completely.
// This is not a good way of doing this.
bool C_ZMHolidayHat::Initialize( C_BaseEntity* pOwner, const char* pszModel )
{
    int modelIndex = UTIL_CreateClientModel( pszModel );
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

void C_ZMHolidayHat::AttachToEntity( C_BaseEntity* pOwner )
{
    // Disables annoying asserts.
    bool lastvalid = C_BaseEntity::IsAbsQueriesValid();
    C_BaseEntity::SetAbsQueriesValid( true );

    FollowEntity( pOwner, true );

    C_BaseEntity::SetAbsQueriesValid( lastvalid );

    UpdateVisibility();
}

//C_BaseAnimating* C_ZMHolidayHat::FindFollowedEntity()
//{
//    return static_cast<C_BaseAnimating*>( GetOwnerEntity() );
//}