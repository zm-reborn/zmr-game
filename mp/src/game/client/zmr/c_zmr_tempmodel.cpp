#include "cbase.h"

#include "c_zmr_tempmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


int UTIL_CreateClientModel( const char* pszModel );


// ZMRTODO: If we ever care enough, we need to re-do this completely.
// This is not a good way of doing this.
bool C_ZMTempModel::Initialize( const char* pszModel )
{
    int modelIndex = UTIL_CreateClientModel( pszModel );
    if ( modelIndex == -1 )
        return false;

    if ( !InitializeAsClientEntityByIndex( modelIndex, RENDER_GROUP_OPAQUE_ENTITY ) )
    {
        return false;
    }


	SetNextClientThink( CLIENT_THINK_NEVER );

    return true;
}
