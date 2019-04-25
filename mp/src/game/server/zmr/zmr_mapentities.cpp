#include "cbase.h"
#include "gameinterface.h"
#include "eventqueue.h"

#include "zmr_mapentities.h"
#include "zmr_mapitemaction.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar zm_sv_debug_mapentities( "zm_sv_debug_mapentities", "0" );


bool FindInList( const char **pStrings, const char *pToFind );


static const char* g_PreserveEnts[] =
{
    "ai_network",
    "ai_hint",
    "hl2mp_gamerules", // Not used.
    "team_manager",
    "player_manager",
    "env_soundscape",
    "env_soundscape_proxy",
    "env_soundscape_triggerable",
    "env_sun",
    "env_wind",
    "env_fog_controller",
    //"func_brush", // Yup, this causes problems (eg. zebra)
    "func_wall",
    "func_buyzone",
    "func_illusionary",
    "infodecal",
    "info_projecteddecal",
    "info_node",
    "info_target",
    "info_node_hint",
    //"info_player_deathmatch",
    //"info_player_combine",
    //"info_player_rebel",
    "info_map_parameters",
    //"keyframe_rope", // Will fuck up maps that parent ropes to objects.
    //"move_rope",
    "info_ladder",
    "player",
    "point_viewcontrol",
    "scene_manager",
    "shadow_control",
    "sky_camera",
    "soundent",
    "trigger_soundscape",
    "viewmodel",
    "predicted_viewmodel",
    "worldspawn",
    "point_devshot_camera",

    // Our preserved ents...
    "zm_gamerules",
    "func_win",
    //"info_player_zombiemaster",
    //"info_player_survivor",
    "info_loadout",

    "zm_objectives_manager",
    "zm_viewmodel",
    "env_fog_controller_zm",

    "vote_controller",

    "", // END Marker
};


#define g_MapEntityRefs     ERROR USE g_ZMMapEntityRefs

static CUtlLinkedList<CMapEntityRef, unsigned short> g_ZMMapEntityRefs;

CZMMapEntitiesSystem g_ZMMapEntities;



//
// Add any special cases here.
//
static bool ShouldMapSystemCreateEntity( const char* pszClassname, CEntityMapData& entData )
{
    // Our item action system (weapon replacement stuff)
    static char temp[MAPKEY_MAXLENGTH];
    if ( ZMItemAction::g_ZMMapItemSystem.ShouldAffectEntity( entData, temp ) )
    {
        return false;
    }

    return true;
}
//
//
//



CZMMapEntitiesSystem::CZMMapEntitiesSystem()
{
    m_bBuildRefList = false;
}

CZMMapEntitiesSystem::~CZMMapEntitiesSystem()
{
}

bool CZMMapEntitiesSystem::ShouldCreateEntity( const char* pszClassname, CEntityMapData& entData )
{
    // Don't recreate the preserved entities unless this is our first time.
    if ( IsPreservedClassname( pszClassname ) )
    {
        return m_bBuildRefList;
    }


    return ShouldMapSystemCreateEntity( pszClassname, entData );
}

CBaseEntity* CZMMapEntitiesSystem::CreateEntity( const char* pszClassname )
{
    // We're building the reference list, step there.
    if ( m_bBuildRefList )
    {
        return BuildListCreate( pszClassname );
    }


    //
    // Okay, all entities that got here should:
    //
    // - Not be preserved ones.
    // - Not be entities with special handling and their own spawning functions. (ie. weapons/ammo in our case)
    //
    //
    // So, the only entities that get here are the ones we can re-spawn and use same edict slot for.
    //
    if ( m_iIterator == g_ZMMapEntityRefs.InvalidIndex() )
    {
        // Oh no! This should not be happening!
        Assert( 0 );
        return nullptr;
    }
    else
    {
        CMapEntityRef& ref = g_ZMMapEntityRefs[m_iIterator];


        // Step to next ent.
        m_iIterator = g_ZMMapEntityRefs.Next( m_iIterator );


        if ( ref.m_iEdict <= 0 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
        {
            // It's possible to get here if we're a server only entity.
            // The "edict" will be 0.
            if ( IsDebugging() && ref.m_iEdict > 0 )
            {
                Msg( "Failed to re-use edict slot! Entity reference slot %i. (Classname %s | Wanted edict: %i)\n", m_iIterator - 1, pszClassname, ref.m_iEdict );
            }

            // Doh! The entity was deleted and its slot was reused.
            // Just use any edict slot. This case sucks because we lose the baseline.
            return CreateEntityByName( pszClassname );
        }
        else
        {
            // Cool, the slot where this entity was is free again (most likely, the entity was 
            // freed above). Now create an entity with this specific index.
            return CreateEntityByName( pszClassname, ref.m_iEdict );
        }
    }
}

CBaseEntity* CZMMapEntitiesSystem::BuildListCreate( const char* pszClassname )
{
    auto* pEnt = CreateEntityByName( pszClassname );


    // Don't save preserved entities to the list.
    if ( IsPreservedClassname( pszClassname ) )
    {
        return pEnt;
    }


    CMapEntityRef ref;
    ref.m_iEdict = -1;
    ref.m_iSerialNumber = -1;

    if ( pEnt )
    {
        ref.m_iEdict = pEnt->entindex();
        if ( pEnt->edict() )
            ref.m_iSerialNumber = pEnt->edict()->m_NetworkSerialNumber;
    }


    g_ZMMapEntityRefs.AddToTail( ref );


    if ( IsDebugging() )
    {
        Msg( "Added entity with classname %s to entity reference list (edict: %i | serial: %i)\n",
            pszClassname,
            ref.m_iEdict,
            ref.m_iSerialNumber );
    }

    return pEnt;
}

const char** CZMMapEntitiesSystem::GetPreserveEntityList()
{
    return g_PreserveEnts;
}

bool CZMMapEntitiesSystem::IsDebugging()
{
    return zm_sv_debug_mapentities.GetBool();
}

bool CZMMapEntitiesSystem::IsPreservedClassname( const char* pszClassname )
{
    return FindInList( GetPreserveEntityList(), pszClassname );
}

void CZMMapEntitiesSystem::InitialSpawn( const char* pMapEntities )
{
    g_ZMMapEntityRefs.Purge();


    m_bBuildRefList = true;
    MapEntity_ParseAllEntities( pMapEntities, reinterpret_cast<IMapEntityFilter*>( &g_ZMMapEntities ) ); // This reinterpret cast is a super hack.
    m_bBuildRefList = false;

    ZMItemAction::g_ZMMapItemSystem.SpawnItems();
}

void CZMMapEntitiesSystem::Restore()
{
    // Get rid of all entities that we don't want to preserve.
    // This will exclude players and other non-map related entities that mappers have no business creating.
    for ( auto* pEnt = gEntList.FirstEnt(); pEnt; pEnt = gEntList.NextEnt( pEnt ) )
    {
        const char* classname = pEnt->GetClassname();

        if ( !IsPreservedClassname( classname ) )
        {
            UTIL_Remove( pEnt );
        }
    }

    // Clear any outputs.
    g_EventQueue.Clear();

    // Really remove the entities so we can have access to their slots below.
    gEntList.CleanupDeleteList();


    //
    // NOTE: THIS IS IMPORTANT!
    //
    // When we're close to edict limit and we remove a bunch of entities,
    // The old free'd edict slots aren't actually free yet!
    // This will ensure that we use any edict slot we can get.
    // I'm unsure whether this causes any side-effects.
    //
    // Best part is, this line is actually in teamplayroundbased_gamerules.cpp
    // but wherever I copied all this from didn't have this line.
    //
    engine->AllowImmediateEdictReuse();



    m_iIterator = g_ZMMapEntityRefs.Head();
    MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), reinterpret_cast<IMapEntityFilter*>( &g_ZMMapEntities ), true ); // This reinterpret cast is a super hack.
    m_iIterator = g_ZMMapEntityRefs.InvalidIndex();



    ZMItemAction::g_ZMMapItemSystem.SpawnItems();
}
