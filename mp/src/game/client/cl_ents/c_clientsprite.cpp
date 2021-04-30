#include "cbase.h"
#include "gamestringpool.h"
#include "cmodel.h"

#include "c_clientsprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



int UTIL_CreateClientModel( const char* pszModel );


C_ClientSprite::C_ClientSprite()
{
}

C_ClientSprite::~C_ClientSprite()
{
}

C_BaseClientEnt* C_ClientSprite::CreateNew()
{
    return new C_ClientSprite;
}

bool C_ClientSprite::Initialize()
{
    if ( NeedsServer() )
    {
        DevMsg( "Sprite has a parent/targetname and cannot be spawned as a client ent!\n" );
        return false;
    }


	if ( InitializeAsClientEntity( STRING( GetModelName() ), RENDER_GROUP_TRANSLUCENT_ENTITY ) == false )
	{
		return false;
	}

    Spawn();

    DevMsg( "Successfully initialized sprite!\n" );

    return true;
}

bool C_ClientSprite::KeyValue( const char* szKeyName, const char* szValue )
{
	if ( FStrEq( szKeyName, "model" ) )
	{
		SetModelName( AllocPooledString( szValue ) );
        return true;
	}
    else if ( FStrEq( szKeyName, "rendermode" ) )
    {
        SetRenderMode( (RenderMode_t)atoi( szValue ) );
        return true;
    }
    else if ( FStrEq( szKeyName, "scale" ) )
    {
        m_flSpriteScale = atof( szValue );
        return true;
    }
    else if ( FStrEq( szKeyName, "GlowProxySize" ) )
    {
        m_flHDRColorScale = atof( szValue );
        return true;
    }
    else if ( FStrEq( szKeyName, "targetname" ) || FStrEq( szKeyName, "parentname" ) )
    {
        // If we have a name or a parent, assume we require updates from server.
        SetNeedsServer();
    }

	return BaseClass::KeyValue( szKeyName, szValue );
}
