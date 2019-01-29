#include "cbase.h"
#include "filesystem.h"
#include "mapentities_shared.h"
#include "mapentities.h"
#include "datacache/imdlcache.h"

#include "zmr_mapitemaction.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_mapitem_debug( "zm_sv_mapitem_debug", "0" );


using namespace ZMItemAction;



#define ITEMACTION_DEFAULT_FILE             "cfg/itemactions_default.txt"
#define ITEMACTION_DEFAULT_FILE_SERVER      "cfg/itemactions_default_server.txt"
#define ITEMACTION_FILE_MAP_FORMAT          "cfg/itemactions_%s.txt"



#define ITEMDATA( classname, flags )             ItemBaseData_t( #classname, flags )
#define CLASSDATA( classname, flag )         ItemClassData_t( #classname, flag )



//
//
//
ItemBaseData_t const CZMMapItemSystem::m_vItemData[] =
{
    // Weapons
    ITEMDATA( weapon_zm_shotgun, CLASS_WEAPON|CLASS_SHOTGUN|CLASS_PRIMARY ),
    ITEMDATA( weapon_zm_mac10, CLASS_WEAPON|CLASS_PRIMARY|CLASS_SMG ),
    ITEMDATA( weapon_zm_rifle, CLASS_WEAPON|CLASS_PRIMARY|CLASS_RIFLE ),
    ITEMDATA( weapon_zm_pistol, CLASS_WEAPON|CLASS_PISTOL|CLASS_SECONDARY ),
    ITEMDATA( weapon_zm_revolver, CLASS_WEAPON|CLASS_PISTOL|CLASS_SECONDARY|CLASS_BIGPISTOL ),

    ITEMDATA( weapon_zm_molotov, CLASS_WEAPON|CLASS_MOLOTOV ),

    ITEMDATA( weapon_zm_improvised, CLASS_WEAPON|CLASS_MELEE ),
    ITEMDATA( weapon_zm_sledge, CLASS_WEAPON|CLASS_MELEE ),

    // Ammo
    ITEMDATA( item_box_buckshot, CLASS_AMMO|CLASS_AMMO_SHOTGUN ),
    ITEMDATA( item_ammo_357, CLASS_AMMO|CLASS_AMMO_RIFLE ),
    ITEMDATA( item_ammo_smg1, CLASS_AMMO|CLASS_AMMO_SMG ),
    ITEMDATA( item_ammo_pistol, CLASS_AMMO|CLASS_AMMO_PISTOL ),
    ITEMDATA( item_ammo_revolver, CLASS_AMMO|CLASS_AMMO_BIGPISTOL ),

    ITEMDATA( item_item_crate, CLASS_ITEM_CRATE ),
};

const ItemClassData_t CZMMapItemSystem::m_vItemClasses[] =
{
    // Weapons
    CLASSDATA( Weapon, CLASS_WEAPON ),

    CLASSDATA( Primary, CLASS_PRIMARY ),
    CLASSDATA( Secondary, CLASS_SECONDARY ),

    CLASSDATA( Shotgun, CLASS_SHOTGUN ),
    CLASSDATA( SMG, CLASS_SMG ),
    CLASSDATA( Rifle, CLASS_RIFLE ),
    CLASSDATA( Pistol, CLASS_PISTOL ),
    CLASSDATA( BigPistol, CLASS_BIGPISTOL ),

    CLASSDATA( Melee, CLASS_MELEE ),

    // Ammo
    CLASSDATA( Ammo, CLASS_AMMO ),

    CLASSDATA( ShotgunAmmo, CLASS_AMMO_SHOTGUN ),
    CLASSDATA( RifleAmmo, CLASS_AMMO_RIFLE ),
    CLASSDATA( SMGAmmo, CLASS_AMMO_SMG ),
    CLASSDATA( PistolAmmo, CLASS_AMMO_PISTOL ),
    CLASSDATA( BigPistolAmmo, CLASS_AMMO_BIGPISTOL ),

    // Misc
    CLASSDATA( Molotov, CLASS_MOLOTOV ),
    CLASSDATA( ItemCrate, CLASS_ITEM_CRATE ),
};

CZMMapItemSystem::CZMMapItemSystem()
{
}

CZMMapItemSystem::~CZMMapItemSystem()
{
    m_vEntData.PurgeAndDeleteElements();
    m_vActions.PurgeAndDeleteElements();
}

void CZMMapItemSystem::LevelInitPreEntity()
{
    m_vActions.PurgeAndDeleteElements();

    if ( !LoadActionsFromFile( ITEMACTION_DEFAULT_FILE_SERVER ) )
    {
        LoadActionsFromFile( ITEMACTION_DEFAULT_FILE );
    }

    LoadActionsFromMapFile();

    

    m_vEntData.PurgeAndDeleteElements();
    LoadItemsFromMap();
    ComputeMapVersion();
}

void CZMMapItemSystem::ComputeMapVersion()
{
    m_bIsOldMap = true;


    static const int indices[] =
    {
        FindItemByClassname( "weapon_zm_shotgun_sporting" ),
    };

    FOR_EACH_VEC( m_vEntData, i )
    {
        int index = FindItemByClassname( m_vEntData[i]->pszItemClassname );

        for ( int j = 0; j < ARRAYSIZE( indices ); j++ )
        {
            Assert( indices[j] != -1 );

            if ( indices[j] == index )
            {
                m_bIsOldMap = false;
                return;
            }
        }
    }
}

bool CZMMapItemSystem::LoadActionsFromFile( const char* filename )
{
    auto* kv = new KeyValues( "ItemActions" );
    
    if ( !kv->LoadFromFile( filesystem, filename ) )
    {
        kv->deleteThis();
        return false;
    }


    for ( auto* action = kv->GetFirstTrueSubKey(); action; action = action->GetNextTrueSubKey() )
    {
        if ( Q_stricmp( "replace", action->GetName() ) == 0 )
        {
            CZMMapItemActionReplace::LoadActions( action, m_vActions );
        }
        else if ( Q_stricmp( "add", action->GetName() ) == 0 )
        {
            CZMMapItemActionAdd::LoadActions( action, m_vActions );
        }
    }


    kv->deleteThis();

    return true;
}

bool CZMMapItemSystem::LoadActionsFromMapFile()
{
    const char* mapname = STRING( gpGlobals->mapname );
    return LoadActionsFromFile( UTIL_VarArgs( ITEMACTION_FILE_MAP_FORMAT, mapname ) );
}

void CZMMapItemSystem::LoadItemsFromMap()
{
    char szTokenBuffer[MAPKEY_MAXLENGTH];

    // Allow the tools to spawn different things
    const char* pMapData = engine->GetMapEntitiesString();
    //if ( serverenginetools )
    //{
    //    pMapData = serverenginetools->GetEntityData( pMapData );
    //}

    // Loop through all entities in the map data and copy the data we want to be able to affect.
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
        if ( !pMapData )
            break;

        if ( token[0] != '{' )
        {
            Warning( "Found %s when expecting {\n", token);
            continue;
        }


        CEntityMapData entData( (char*)pMapData );

        if ( !ShouldAffectEntity( entData, token ) )
        {
            continue;
        }


        auto* pEntData = ItemEntData_t::Create( pMapData );
        if ( pEntData )
        {
            m_vEntData.AddToTail( pEntData );
        }
    }
}

unsigned int CZMMapItemSystem::GetClassFlag( const char* classname )
{
    int index = FindClassByName( classname );

    if ( index != -1 )
    {
        return m_vItemClasses[index].m_fClassFlag;
    }
    
    return 0;
}

unsigned int CZMMapItemSystem::GetItemFlags( const char* classname )
{
    int index = FindItemByClassname( classname );

    if ( index != -1 )
    {
        return m_vItemData[index].m_fClassFlags;
    }
    
    return 0;
}

bool CZMMapItemSystem::GetMapItemsByClass( unsigned int flags, CUtlVector<const ItemBaseData_t*>& items )
{
    int num = 0;

    for ( int i = 0; i < ARRAYSIZE( m_vItemData ); i++ )
    {
        if ( m_vItemData[i].m_fClassFlags & flags )
        {
            items.AddToTail( &m_vItemData[i] );
            ++num;
        }
    }

    return num > 0;
}

int CZMMapItemSystem::FindClassByName( const char* classname )
{
    for ( int i = 0; i < ARRAYSIZE( m_vItemClasses ); i++ )
    {
        if ( Q_stricmp( m_vItemClasses[i].m_pszClassname, classname ) == 0 )
        {
            return i;
        }
    }

    return -1;
}

int CZMMapItemSystem::FindItemByClassname( const char* classname )
{
    for ( int i = 0; i < ARRAYSIZE( m_vItemData ); i++ )
    {
        if ( Q_stricmp( m_vItemData[i].m_pszClassname, classname ) == 0 )
        {
            return i;
        }
    }

    return -1;
}

const char* CZMMapItemSystem::OnCreateItem( const char* classname )
{
    int iItemIndex = FindItemByClassname( classname );
    if ( iItemIndex == -1 )
    {
        return classname;
    }


    ItemEntData_t itemdata(
        m_vItemData[iItemIndex].m_pszClassname,
        m_vItemData[iItemIndex].m_fClassFlags );


    bool bAffected = false;

    FOR_EACH_VEC( m_vActions, i )
    {
        if ( m_vActions[i]->AffectsItem( itemdata, TIME_ENTSPAWN ) )
        {
            bAffected = true;
            break;
        }
    }


    // We don't need to do anything to this item.
    if ( !bAffected )
    {
        return classname;
    }



    ZMMapData_t mapdata;
    mapdata.bIsOldMap = m_bIsOldMap;
    mapdata.items.AddToTail( &itemdata );


    FOR_EACH_VEC( m_vActions, i )
    {
        m_vActions[i]->PerformAction( mapdata, TIME_ENTSPAWN );
    }


    return itemdata.pszItemClassname;
}

void CZMMapItemSystem::GetCopiedData( ZMItems_t& items )
{
    FOR_EACH_VEC( m_vEntData, i )
    {
        items.AddToTail( ItemEntData_t::Create( m_vEntData[i]->pszEntData ) );
    }
}

void CZMMapItemSystem::SpawnItems()
{
    ZMMapData_t mapdata;
    mapdata.bIsOldMap = m_bIsOldMap;


    GetCopiedData( mapdata.items );
    int len = mapdata.items.Count();
    if ( len < 1 )
        return;


    FOR_EACH_VEC( m_vActions, i )
    {
        m_vActions[i]->PerformAction( mapdata, TIME_WORLDRESET );
    }


    CBaseEntity** pEnts = new CBaseEntity*[len];
    int nEnts = 0;

    for ( int i = 0; i < len; i++ )
    {
        auto* pEntData = mapdata.items[i];


        auto* pEnt = CreateEntityByName( pEntData->pszItemClassname );

        if ( !pEnt )
            continue;


        CEntityMapData entData( (char*)pEntData->pszEntData );
        pEnt->ParseMapData( &entData );
        if ( DispatchSpawn( pEnt ) < 0 )
        {
            continue;
        }

        pEnts[nEnts++] = pEnt;
    }


    mapdata.items.PurgeAndDeleteElements();


    bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
    for ( int i = 0; i < nEnts; i++ )
    {
        MDLCACHE_CRITICAL_SECTION();
        pEnts[i]->Activate();
    }
    mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );


    delete[] pEnts;
}

bool CZMMapItemSystem::ShouldAffectEntity( CEntityMapData& entData, char* buffer )
{
    // Do we want to affect this entity in the first place?
    if ( !entData.ExtractValue( "classname", buffer ) )
        return false;

    if ( !AffectsItem( buffer ) )
        return false;


    // We don't want to touch entities that may be part of some map logic.

    if ( entData.ExtractValue( "targetname", buffer ) && buffer[0] != NULL )
        return false;

    if ( entData.ExtractValue( "parent", buffer ) && buffer[0] != NULL )
        return false;


    return true;
}

const ItemBaseData_t* CZMMapItemSystem::GetItemData( int index )
{
    if ( index >= 0 && index < ARRAYSIZE( m_vItemData ) )
    {
        return &m_vItemData[index];
    }

    return nullptr;
}

bool CZMMapItemSystem::AffectsItem( const char* classname )
{
    return FindItemByClassname( classname ) != -1;
}
//
//
//



//
//
//
CZMMapItemActionReplace::CZMMapItemActionReplace()
{
    m_flChanceFrac = -1.0f;
    m_flReplaceFrac = -1.0f;


    m_fFlag = 0;
    m_szClassname[0] = NULL;


    m_flRangeCheck = 0.0f;
    m_fFilterFlags = 0;
    m_iFilterItemIndex = -1;

    m_bMapItemsOnly = true;
}

CZMMapItemActionReplace::~CZMMapItemActionReplace()
{
    m_vSubReplaces.PurgeAndDeleteElements();
}

bool CZMMapItemActionReplace::CalcChance()
{
    if ( m_flChanceFrac >= 0.0f && m_flChanceFrac < 1.0f )
    {
        float c = random->RandomFloat();
        if ( c > m_flChanceFrac )
        {
            return false;
        }
    }

    return true;
}

void CZMMapItemActionReplace::ReplacePerc( ZMItems_t& items )
{
    // Remove some of items from our list

    if ( m_flReplaceFrac < 0.0f || m_flReplaceFrac >= 1.0f )
        return;


    int total = items.Count();
    int remove = (1.0f - m_flReplaceFrac) * total;

    for ( int i = 0; i < remove; i++ )
    {
        items.Remove( random->RandomInt( 0, items.Count() - 1 ) );
    }
}

int CZMMapItemActionReplace::PerformAction( ZMMapData_t& mapdata, ItemSpawnTime_t status )
{
    if ( !CalcChance() )
    {
        return 0;
    }

    if ( m_bOnlyOldMaps && !mapdata.bIsOldMap )
    {
        return 0;
    }


    int nChanged = 0;

    {
        // Do the replacing

        ZMItems_t myitems;

        FOR_EACH_VEC( mapdata.items, i )
        {
            if ( AffectsItem( *mapdata.items[i], status ) )
            {
                myitems.AddToTail( mapdata.items[i] );
            }
        }
    

        if ( myitems.Count() )
        {
            ReplacePerc( myitems );

            FOR_EACH_VEC( myitems, i )
            {
                Replace( *myitems[i] );
                ++nChanged;
            }
        }
    }


    FOR_EACH_VEC( m_vSubReplaces, i )
    {
        nChanged += m_vSubReplaces[i]->PerformAction( mapdata, status );
    }

    return nChanged;
}

bool CZMMapItemActionReplace::AffectsItem( ItemEntData_t& itemEntData, ItemSpawnTime_t status ) const
{
    if ( m_flRangeCheck > 0.0f && m_vecRangeCheckPos.DistToSqr( itemEntData.vecPos ) > (m_flRangeCheck*m_flRangeCheck) )
    {
        return false;
    }


    // A specific item needs to be affected
    if ( m_iFilterItemIndex != -1 )
    {
        if ( CZMMapItemSystem::FindItemByClassname( itemEntData.pszItemClassname ) != m_iFilterItemIndex )
            return false;
    }
    // It's a class, use the flags
    else
    {
        unsigned int res = m_fFilterFlags & itemEntData.fClassFlags;

        if ( !m_fFilterFlags || res != m_fFilterFlags )
            return false;
    }


    if ( m_bMapItemsOnly && status != TIME_WORLDRESET )
    {
        return false;
    }


    return true;
}

bool CZMMapItemActionReplace::Replace( ItemEntData_t& itemEntData )
{
    char classname[64];
    classname[0] = NULL;


    // Do the actual edit
    if ( m_szClassname[0] != NULL )
    {
        // Specific item classname
        Q_strncpy( classname, m_szClassname, sizeof( classname ) );
    }
    else if ( m_fFlag != 0 )
    {
        // Pick a random item
        CUtlVector<const ItemBaseData_t*> items;
        CZMMapItemSystem::GetMapItemsByClass( m_fFlag, items );
        if ( items.Count() > 0 )
        {
            Q_strncpy(
                classname,
                items[random->RandomInt( 0, items.Count() - 1)]->m_pszClassname,
                sizeof( classname ) );
        }
    }


    if ( Q_stricmp( itemEntData.pszItemClassname, classname ) != 0 )
    {
        if ( itemEntData.pszEntData )
        {
            CEntityMapData entData( (char*)itemEntData.pszEntData, itemEntData.iEntDataSize );

            entData.SetValue( "classname", classname );
        }



        if ( zm_sv_mapitem_debug.GetBool() )
        {
            Msg( "Replaced '%s' with '%s'\n", itemEntData.pszItemClassname, classname );
        }


        // Update our temporary data
        int iItemIndex = CZMMapItemSystem::FindItemByClassname( classname );
        if ( iItemIndex != -1 )
        {
            itemEntData.pszItemClassname = CZMMapItemSystem::m_vItemData[iItemIndex].m_pszClassname;
            itemEntData.fClassFlags = CZMMapItemSystem::m_vItemData[iItemIndex].m_fClassFlags;
        }
        else
        {
            itemEntData.pszItemClassname = "";
            itemEntData.fClassFlags = 0;
        }

        return true;
    }
    

    return false;
}

CZMMapItemActionReplace* CZMMapItemActionReplace::Create( KeyValues* kv )
{
    if ( zm_sv_mapitem_debug.GetBool() )
    {
        Msg( "Loading replace '%s'\n", kv->GetName() );
    }


    auto* pReplace = new CZMMapItemActionReplace;

    float chance = kv->GetFloat( "chance", -1.0f );

    if ( chance >= 0.0f )
        pReplace->m_flChanceFrac = chance;


    float replace_frac = kv->GetFloat( "replace_frac", -1.0f );

    if ( replace_frac >= 0.0f )
    {
        pReplace->m_flReplaceFrac = replace_frac;
    }



    pReplace->m_bOnlyOldMaps = kv->GetBool( "onlyoldmaps", false );

    pReplace->m_bMapItemsOnly = kv->GetBool( "mapitemsonly", true );



    pReplace->ParseReplaceFilter( kv );


    const char* pszReplace = kv->GetString( "replace", nullptr );
    if ( !pszReplace )
    {
        delete pReplace;
        return nullptr;
    }


    pReplace->ParseReplaceTarget( pszReplace );



    for ( auto* subkv = kv->GetFirstTrueSubKey(); subkv; subkv = subkv->GetNextTrueSubKey() )
    {
        auto* sub = CZMMapItemActionReplace::Create( subkv );

        if ( sub )
        {
            pReplace->m_vSubReplaces.AddToTail( sub );
        }
    }
    

    return pReplace;
}

bool CZMMapItemActionReplace::ParseReplaceTarget( const char* target )
{
    auto flag = CZMMapItemSystem::GetClassFlag( target );

    if ( flag )
    {
        m_fFlag = flag;
        return true;
    }


    if ( CZMMapItemSystem::AffectsItem( target ) )
    {
        Q_strncpy( m_szClassname, target, sizeof( m_szClassname ) );
    }


    return true;
}

bool CZMMapItemActionReplace::ParseReplaceFilter( KeyValues* kv )
{
    const char* classfilter = kv->GetString( "filter_class" );
    
    if ( !*classfilter )
    {
        return false;
    }


    m_fFilterFlags = CZMMapItemSystem::GetClassFlag( classfilter );
    if ( !m_fFilterFlags )
    {
        m_iFilterItemIndex = CZMMapItemSystem::FindItemByClassname( classfilter );
        
        if ( m_iFilterItemIndex == -1 )
        {
            return false;
        }
    }


    char buffer[256];

    
    const char* rangefilter = kv->GetString( "filter_range" );
    if ( *rangefilter )
    {
        sscanf( buffer, "%f;%f,%f,%f", &m_flRangeCheck, &m_vecRangeCheckPos.x, &m_vecRangeCheckPos.y, &m_vecRangeCheckPos.z );
    }



    return true;
}

void CZMMapItemActionReplace::LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions )
{
    for ( auto* rep = kv->GetFirstTrueSubKey(); rep; rep = rep->GetNextTrueSubKey() )
    {
        auto* pRep = CZMMapItemActionReplace::Create( rep );
        if ( pRep )
        {
            actions.AddToTail( pRep );
        }
    }
}
//
//
//



//
//
//
CZMMapItemActionAdd::CZMMapItemActionAdd()
{

}


CZMMapItemActionAdd::~CZMMapItemActionAdd()
{
    
}

void CZMMapItemActionAdd::LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions )
{
    for ( auto* rep = kv->GetFirstTrueSubKey(); rep; rep = rep->GetNextTrueSubKey() )
    {
        auto* pRep = CZMMapItemActionAdd::Create( rep );
        if ( pRep )
        {
            actions.AddToTail( pRep );
        }
    }
}

CZMMapItemActionAdd* CZMMapItemActionAdd::Create( KeyValues* kv )
{
    return nullptr;
}
//
//
//



//
//
//
ItemEntData_t* ItemEntData_t::Create( const char* pszEntData )
{
    char token[MAPKEY_MAXLENGTH];
    CEntityMapData entData( (char*)pszEntData );
    Vector pos;
    unsigned int flags;



    if ( !entData.ExtractValue( "classname", token ) || token[0] == NULL )
        return nullptr;



    int iItemIndex = CZMMapItemSystem::FindItemByClassname( token );
    if ( iItemIndex == -1 )
        return nullptr;


    const char* classname = CZMMapItemSystem::m_vItemData[iItemIndex].m_pszClassname;
    flags = CZMMapItemSystem::m_vItemData[iItemIndex].m_fClassFlags;
    if ( !flags )
        return nullptr;


    if ( !entData.ExtractValue( "origin", token ) || token[0] == NULL )
        return nullptr;


    sscanf( token, "%f %f %f", &pos.x, &pos.y, &pos.z );



    // Skip to the end of the entity data to get the size of it.
    char keyName[MAPKEY_MAXLENGTH];
    bool ok;
    for ( ok = entData.GetFirstKey( keyName, token ); ok; ok = entData.GetNextKey( keyName, token ) )
    {
    }


    const char* pszEnd = entData.CurrentBufferPosition();
    if ( !pszEnd )
        return nullptr;


    size_t size = (size_t)(pszEnd - pszEntData) + 1;
    if ( size <= 1 )
        return nullptr;


    size_t contentsize = size;
    size += 128;

    char* pszNewEntData = new char[size];
    Q_strncpy( pszNewEntData, pszEntData, contentsize+2 );


    return new ItemEntData_t( pszNewEntData, (int)size, flags, pos, classname );
}
//
//
//


CZMMapItemSystem ZMItemAction::g_ZMMapItemSystem;
