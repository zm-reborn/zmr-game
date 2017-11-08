#include "cbase.h"
#include "mapentities_shared.h"


#include "c_cliententitysystem.h"

#include "c_clientsprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CClientEntitySystem g_ClientEntitySystem;



CClientEntitySystem::CClientEntitySystem() : CAutoGameSystem( "CClientEntitySystem" )
{
	
}

CClientEntitySystem::~CClientEntitySystem()
{

}

void CClientEntitySystem::PostInit()
{
	RegisterClientEnts();
}

void CClientEntitySystem::AddClientSideEntityListener( const char* pszClassname, CLIENTENTCREATENEW fn )
{
	m_vClientEntRegister.AddToTail( { pszClassname, fn } );
}

void CClientEntitySystem::RecreateAll()
{
    DestroyAll();

    ParseAllEntities( engine->GetMapEntitiesString() );
}

void CClientEntitySystem::DestroyAll()
{
    while ( m_vClientEnts.Count() > 0 )
    {
        m_vClientEnts[0]->Release();
    }
}

void CClientEntitySystem::ParseAllEntities( const char* pMapData )
{
	int nEntities = 0;

	char szTokenBuffer[MAPKEY_MAXLENGTH];

	//
	//  Loop through all entities in the map data, creating each.
	//
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Error( "MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		//
		// Parse the entity and add it to the spawn list.
		//

		pMapData = ParseEntity( pMapData );

		nEntities++;
	}
}

const char* CClientEntitySystem::ParseEntity( const char* pEntData )
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	MDLCACHE_CRITICAL_SECTION();

	if ( !entData.ExtractValue( "classname", className ) )
	{
		Error( "Classname missing from entity while trying to spawn client-side entities!\n" );
	}


	//
	// Check if this entity is a client-side entity
	// and spawn it.
	//
    int len = m_vClientEntRegister.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( Q_strcmp( className, m_vClientEntRegister[i].classname ) == 0 )
        {
            C_BaseClientEnt* pEnt = (*m_vClientEntRegister[i].pfn)();

		    if ( pEnt )
		    {	// Set up keyvalues.
			    pEnt->ParseMapData( &entData );
			
			    if ( !pEnt->Initialize() )
				    pEnt->Release();
		
			    return entData.CurrentBufferPosition();
		    }
        }
    }
	

	// Just skip past all the keys.
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];
	if ( entData.GetFirstKey(keyName, value) )
	{
		do 
		{
		} 
		while ( entData.GetNextKey(keyName, value) );
	}


	return entData.CurrentBufferPosition();
}
//



//
C_BaseClientEnt::C_BaseClientEnt()
{
	m_iNetworkStatus = ENS_CLIENTSIDE;

	g_ClientEntitySystem.m_vClientEnts.AddToTail( this );
}

C_BaseClientEnt::~C_BaseClientEnt()
{
    g_ClientEntitySystem.m_vClientEnts.FindAndRemove( this );
}
//



//
void RegisterClientEnts()
{
	//
	// Register client-side entities here.
	//
    REGISTER_CLIENTENT( env_sprite, C_ClientSprite::CreateNew );
    REGISTER_CLIENTENT( env_sprite_clientside, C_ClientSprite::CreateNew );
}

