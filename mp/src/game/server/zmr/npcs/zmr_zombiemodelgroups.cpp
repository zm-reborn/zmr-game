#include "cbase.h"
#include <filesystem.h>

#include "zmr_zombiemodelgroups.h"



#define DEF_MODELGROUP                  "DefaultModels"

#define DATAFILE_NAME                   "zombiemodelgroups"
#define DATAFILE_PATH                   "scripts/"DATAFILE_NAME".txt"

#define DATAFILE_PATH_MAP_FORMAT        "scripts/"DATAFILE_NAME"_%s.txt"



CZMZombieModelGroupSystem g_ZombieModelGroups;


//
//
//
CZMZombieModelData::CZMZombieModelData( KeyValues* kv )
{
    m_iPrecacheIndex = -1;


    m_sName = AllocPooledString( kv->GetName() );
    m_sModelPath = NULL_STRING;


    CopyData( kv );
}

CZMZombieModelData::~CZMZombieModelData()
{
}

void CZMZombieModelData::CopyData( KeyValues* kv )
{
    const char* modelpath = kv->GetString( "path", nullptr );
    if ( modelpath )
    {
        m_sModelPath = AllocPooledString( modelpath );
    }


    
    int skin = kv->GetInt( "skin", -1 );
    if ( kv->GetBool( "randomskin" ) )
    {
        // Pick any skin
        m_fSkins = 0xFFFFFFFF;
        
    }
    else if ( skin >= 0 )
    {
        m_fSkins = (1 << skin);
    }
    else
    {
        m_fSkins = 1;
    }


    int bodygroup = kv->GetInt( "bodygroup", -1 );
    if ( kv->GetBool( "randombodygroup" ) )
    {
        // Pick any bodygroup
        m_fBodygroups = 0xFFFFFFFF;
    }
    else if ( bodygroup >= 0 )
    {
        m_fBodygroups = (1 << bodygroup);
    }
    else
    {
        m_fBodygroups = 1;
    }
}

bool CZMZombieModelData::IsValidModel( const char* mdlpath )
{
    if ( !mdlpath || !(*mdlpath) )
        return false;


    return filesystem->FileExists( mdlpath );
}

bool CZMZombieModelData::PrecacheMe()
{
    m_iPrecacheIndex = CBaseEntity::PrecacheModel( GetModelPath() );
    
    return m_iPrecacheIndex != -1;
}


//
//
//
CZMZombieDataPerType::CZMZombieDataPerType()
{
}

CZMZombieDataPerType::~CZMZombieDataPerType()
{
    m_vModels.PurgeAndDeleteElements();
}

void CZMZombieDataPerType::AddModelData( KeyValues* kv )
{
    int found = FindModelByName( kv->GetName() );
    if ( found != -1 )
    {
        m_vModels[found]->CopyData( kv );
        return;
    }

    m_vModels.AddToTail( new CZMZombieModelData( kv ) );
}

int CZMZombieDataPerType::FindModelByName( const char* name ) const
{
    FOR_EACH_VEC( m_vModels, i )
    {
        if ( Q_stricmp( name, m_vModels[i]->GetModelDataName() ) == 0 )
        {
            return i;
        }
    }
    
    return -1;
}

void CZMZombieDataPerType::CopyData( KeyValues* kv )
{
    KeyValues* mdl = kv->GetFirstSubKey();
    if ( !mdl )
        return;

    do
    {
        AddModelData( mdl );
    }
    while ( (mdl = mdl->GetNextKey()) != nullptr );
}

void CZMZombieDataPerType::LoadSection( KeyValues* kv )
{
    if ( !kv )
        return;



    KeyValues* mdldata = kv->GetFirstSubKey();
    if ( !mdldata )
        return;


    do
    {
        if ( !(*mdldata->GetName()) )
        {
            continue;
        }

        AddModelData( mdldata );
    }
    while ( (mdldata = mdldata->GetNextKey()) != nullptr );
}

bool CZMZombieDataPerType::PrecacheMe()
{
    FOR_EACH_VEC( m_vModels, i )
    {
        auto* pModelData = m_vModels[i];
        if ( !pModelData->PrecacheMe() )
        {
            Warning( "Failed to precache zombie model group! Model: %s!\n", pModelData->GetModelPath() );
            
            m_vModels.Remove( i );
            --i;

            delete pModelData;
        }
    }

    return m_vModels.Count() > 0;
}


//
//
//
CZMZombieModelGroup::CZMZombieModelGroup( KeyValues* kv )
{
    const char* name = kv->GetName();

    m_sModelGroup = AllocPooledString( name );

    CopyData( kv );
}

CZMZombieModelGroup::~CZMZombieModelGroup()
{
    for ( int i = 0; i < ARRAYSIZE( m_Data ); i++ )
        m_Data[i].~CZMZombieDataPerType();
}

void CZMZombieModelGroup::CopyData( KeyValues* kv )
{
    m_Data[ZMCLASS_SHAMBLER].LoadSection( kv->FindKey( "Shambler" ) );
    m_Data[ZMCLASS_BANSHEE].LoadSection( kv->FindKey( "Banshee" ) );
    m_Data[ZMCLASS_HULK].LoadSection( kv->FindKey( "Hulk" ) );
    m_Data[ZMCLASS_DRIFTER].LoadSection( kv->FindKey( "Drifter" ) );
    m_Data[ZMCLASS_IMMOLATOR].LoadSection( kv->FindKey( "Immolator" ) );
}

const CZMZombieModelData* CZMZombieModelGroup::RandomModel( ZombieClass_t zclass ) const
{
    auto& mdls = m_Data[zclass].m_vModels;
    int l = mdls.Count();
    if ( l < 1 )
        return nullptr;

    return mdls[random->RandomInt( 0, l - 1 )];
}



//
//
//
CZMZombieModelGroupSystem::CZMZombieModelGroupSystem() : CAutoGameSystem( "ZMZombieDataSystem" )
{
}

CZMZombieModelGroupSystem::~CZMZombieModelGroupSystem()
{
}

void CZMZombieModelGroupSystem::LevelInitPreEntity()
{
    SetDefaultModelGroup( DEF_MODELGROUP );

    LoadFiles();

    FOR_EACH_VEC( m_vModelGroups, i )
    {
        PrecacheModelGroup( m_vModelGroups[i]->GetModelGroupName() );
    }
}

bool CZMZombieModelGroupSystem::Init()
{
    return true;
}

void CZMZombieModelGroupSystem::Reload()
{
    LevelInitPreEntity();
}

void CZMZombieModelGroupSystem::LoadFiles()
{
    m_vModelGroups.PurgeAndDeleteElements();

    KeyValues* kv;


    // Load default settings
    kv = new KeyValues( "ModelGroups" );

    if ( !kv->LoadFromFile( filesystem, DATAFILE_PATH ) )
    {
        kv->deleteThis();
        Warning( "Couldn't open zombie data file! (%s)\n", DATAFILE_PATH );
        return;
    }


    LoadFromFile( kv );


    kv->deleteThis();




    // Load map specific settings
    kv = new KeyValues( "ModelGroups" );
    const char* mapname = STRING( gpGlobals->mapname );

    if ( !kv->LoadFromFile( filesystem, UTIL_VarArgs( DATAFILE_PATH_MAP_FORMAT, mapname ) ) )
    {
        kv->deleteThis();
        return;
    }


    LoadFromFile( kv );

    kv->deleteThis();
}

void CZMZombieModelGroupSystem::LoadFromFile( KeyValues* kv )
{
    KeyValues* temp;
    
    temp = kv->FindKey( "default_modelgroup" );
    if ( temp )
        SetDefaultModelGroup( temp->GetString() );


    KeyValues* mdlgroup = kv->GetFirstTrueSubKey();
    if ( !mdlgroup )
        return;

    do
    {
        if ( !(*mdlgroup->GetName()) )
        {
            continue;
        }

        AddModelGroup( mdlgroup );
    }
    while ( (mdlgroup = mdlgroup->GetNextTrueSubKey()) != nullptr );
}

void CZMZombieModelGroupSystem::OnZombieSpawn( CZMBaseZombie* pZombie )
{
    const CZMZombieModelData* data;

    ZombieClass_t zclass = pZombie->GetZombieClass();
    const char* mdlgroup = pZombie->GetZombieModelGroupName();
    data = PickRandomModelFromGroup( zclass, mdlgroup );

    if ( data )
    {
        const char* model = data->GetModelPath();

        if ( !engine->IsModelPrecached( model ) )
        {
            PrecacheModelGroup( mdlgroup );
            CBaseEntity::PrecacheModel( model );
        }

        pZombie->SetModel( model );


        CStudioHdr* hdr = pZombie->GetModelPtr();

        if ( hdr )
        {
            // Set skin
            CUtlVector<int> vSkins;

            int flags = data->GetSkinBitFlag();
            int num = MIN( hdr->numskinfamilies(), (sizeof( flags ) * 8) );
            for ( int i = 0; i < num; i++ )
            {
                int f = ( 1 << i );
                if ( flags & f )
                    vSkins.AddToTail( i );
            }


            if ( vSkins.Count() > 0 )
            {
                pZombie->m_nSkin = vSkins[random->RandomInt( 0, vSkins.Count() - 1 )];
            }
            else
            {
                pZombie->m_nSkin = 0;
            }

            //pZombie->SetBodygroup()
        }
    }
    else
    {
        Warning( "Failed to find model group for zombie!\n" );

#define ERROR_MODEL     "models/error.mdl"

        if ( CBaseEntity::PrecacheModel( ERROR_MODEL ) )
        {
            pZombie->SetModel( ERROR_MODEL );
        }
    }
}

void CZMZombieModelGroupSystem::SetDefaultModelGroup( const char* group )
{
    m_sDefaultModelGroup = AllocPooledString( group );
}

void CZMZombieModelGroupSystem::AddModelGroup( KeyValues* kv )
{
    int found = FindModelGroupByName( kv->GetName() );
    if ( found != -1 )
    {
        m_vModelGroups[found]->CopyData( kv );
        return;
    }

    m_vModelGroups.AddToTail( new CZMZombieModelGroup( kv ) );
}

int CZMZombieModelGroupSystem::FindModelGroupByName( const char* name ) const
{
    if ( !name || !(*name) )
        return -1;

    FOR_EACH_VEC( m_vModelGroups, i )
    {
        if ( Q_stricmp( name, m_vModelGroups[i]->GetModelGroupName() ) == 0 )
        {
            return i;
        }
    }
    
    return -1;
}

const CZMZombieModelData* CZMZombieModelGroupSystem::PickRandomModelFromGroup( ZombieClass_t zclass, const char* group ) const
{
    const CZMZombieModelData* data;

    int index = FindModelGroupByName( group );
    data = ( index != -1 ) ? m_vModelGroups[index]->RandomModel( zclass ) : nullptr;
    if ( !data )
    {
        // Try default model group
        index = FindModelGroupByName( GetDefaultModelGroup() );
        data = ( index != -1 ) ? m_vModelGroups[index]->RandomModel( zclass ) : nullptr;
        if ( !data )
        {
            index = FindModelGroupByName( DEF_MODELGROUP );
            data = ( index != -1 ) ? m_vModelGroups[index]->RandomModel( zclass ) : nullptr;

            if ( !data )
                return nullptr;
        }
    }


    return data;
}

void CZMZombieModelGroupSystem::PrecacheModelGroup( const char* group )
{
    int index = FindModelGroupByName( group );
    if ( index == -1 )
        return;

    auto* mdlgroup = m_vModelGroups[index];

    
    for ( int i = 0; i < ARRAYSIZE( mdlgroup->m_Data ); i++ )
    {
        mdlgroup->m_Data[i].PrecacheMe();
    }
}

bool CZMZombieModelGroupSystem::PrecacheModelGroup( ZombieClass_t zclass, const char* group )
{
    int index = FindModelGroupByName( group );
    if ( index == -1 )
    {
        return false;
    }


    m_vModelGroups[index]->m_Data[zclass].PrecacheMe();

    return true;
}

CON_COMMAND( zm_zombiemodelgroups_reload, "Reloads zombie model groups" )
{
    g_ZombieModelGroups.Reload();
}
