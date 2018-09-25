#include "cbase.h"
#include "filesystem.h"

#include <networkstringtabledefs.h>


#include "zmr_playermodels.h"



#define PLAYERMODEL_FILE            "resource/zmplayermodels.txt"
#define PLAYERMODEL_FILE_CUSTOM     "resource/zmplayermodels_custom.txt"

#define DEF_PLAYER_MODEL            "models/male_pi.mdl"



INetworkStringTable* g_pZMCustomPlyModels = nullptr;


//
CZMPlayerModelData::CZMPlayerModelData( const KeyValues* kv, bool bIsCustom )
{
    Assert( kv );
    m_pKvData = kv->MakeCopy();
    m_szModelName = m_pKvData->GetString( "model" );

    m_bIsCustom = bIsCustom;
}

CZMPlayerModelData::~CZMPlayerModelData()
{
    if ( m_pKvData )
        m_pKvData->deleteThis();
    m_pKvData = nullptr;
}

KeyValues* CZMPlayerModelData::CreateEmptyModelData( const char* model, const char* name )
{
    KeyValues* kv = new KeyValues( name );
    kv->SetString( "model", model );

    return kv;
}
//


//
CZMPlayerModelSystem::CZMPlayerModelSystem()
{
#ifdef GAME_DLL
    m_bLoadedFromFile = false;
#endif
}

CZMPlayerModelSystem::~CZMPlayerModelSystem()
{
    m_vPlayerModels.PurgeAndDeleteElements();
}

#ifdef GAME_DLL
void CZMPlayerModelSystem::LevelInitPreEntity()
{
    m_vPlayerModels.PurgeAndDeleteElements();
    m_bLoadedFromFile = false;
}
#endif

const char* CZMPlayerModelSystem::GetDefaultPlayerModel()
{
    return DEF_PLAYER_MODEL;
}

void CZMPlayerModelSystem::ParseCustomModels( ZMPlayerModelList_t& list )
{
    if ( !g_pZMCustomPlyModels )
        return;


    char buffer[512];

    int nMdls = g_pZMCustomPlyModels->GetNumStrings();
    for ( int i = 0; i < nMdls; i++ )
    {
        Q_strncpy( buffer, g_pZMCustomPlyModels->GetString( i ), sizeof( buffer ) );

        char* sep = Q_strrchr( buffer, ';' );
        if ( !sep )
            continue;


        *sep = 0;

        KeyValues* kv = CZMPlayerModelData::CreateEmptyModelData( sep + 1, buffer );

        list.AddToTail( new CZMPlayerModelData( kv, true ) );

        kv->deleteThis();
    }
}

void CZMPlayerModelSystem::SaveCustomModelsToStringTable()
{
    // There's no way to reset a string table?
    if ( g_pZMCustomPlyModels->GetNumStrings() != 0 )
    {
        return;
    }


    int num = 0;

    char buffer[512];

    int len = m_vPlayerModels.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vPlayerModels[i]->IsCustom() )
        {
            KeyValues* kv = m_vPlayerModels[i]->GetModelData();

            Q_snprintf( buffer, sizeof( buffer ), "%s;%s", kv->GetName(), kv->GetString( "model" ) );

            g_pZMCustomPlyModels->AddString( true, buffer );

            ++num;
        }
    }

    DevMsg( "Wrote %i custom player models to string table.\n", num );
}

int CZMPlayerModelSystem::LoadModelsFromFile()
{
#ifdef GAME_DLL
    // Only load once per map
    if ( m_bLoadedFromFile )
        return m_vPlayerModels.Count();
#endif


#ifdef CLIENT_DLL
    // Only load once from file for client.
    if ( m_vPlayerModels.Count() )
        return m_vPlayerModels.Count();
#endif


    LoadStockModels();
#ifdef GAME_DLL
    LoadCustomModels();
    SaveCustomModelsToStringTable();
#endif

#ifdef GAME_DLL
    // We need to add something for the server.
    if ( !m_vPlayerModels.Count() )
    {
        AddFallbackModel();
    }


    DevMsg( "Total player models: %i\n", m_vPlayerModels.Count() );


    m_bLoadedFromFile = true;
#endif

    return m_vPlayerModels.Count();
}

int CZMPlayerModelSystem::LoadStockModels()
{
    KeyValues* kv = new KeyValues( "PlayerModels" );
    if ( !kv->LoadFromFile( filesystem, PLAYERMODEL_FILE ) )
    {
        kv->deleteThis();
        return 0;
    }


    int ret = LoadModelData( kv, m_vPlayerModels, false );

    kv->deleteThis();

    return ret;
}

int CZMPlayerModelSystem::LoadCustomModels()
{
    KeyValues* kv = new KeyValues( "PlayerModels" );
    if ( !kv->LoadFromFile( filesystem, PLAYERMODEL_FILE_CUSTOM ) )
    {
        kv->deleteThis();
        return 0;
    }


    int ret = LoadModelData( kv, m_vPlayerModels, true );

    kv->deleteThis();


    DevMsg( "Loaded %i custom player models from file!\n", ret );

    return ret;
}

int CZMPlayerModelSystem::PrecachePlayerModels()
{
    int len = m_vPlayerModels.Count();
    for ( int i = 0; i < len; i++ )
    {
        CZMPlayerModelData* pData = m_vPlayerModels[i];
        const char* model = pData->GetModelName();
        const char* name = pData->GetModelData()->GetName();

        if ( CBaseEntity::PrecacheModel( model ) != -1 ) // Valid model?
        {
            const char* arms = pData->GetArmModel();
            if ( *arms )
            {
                if ( CBaseEntity::PrecacheModel( arms ) != -1 )
                {
                    DevMsg( "Precached player model arm model (%s) explicitly.\n", arms );
                }
                else
                {
                    pData->GetModelData()->SetString( "armsmodel", "" );
                    Warning( "Invalid arm model path '%s' ('%s')!\n", arms, name );
                }
            }
        }
        else
        {
            delete m_vPlayerModels[i];
            m_vPlayerModels.Remove( i );
            --i;
        }
    }

    return m_vPlayerModels.Count();
}

int CZMPlayerModelSystem::LoadModelData( KeyValues* kv, ZMPlayerModelList_t& list, bool bIsCustom )
{
    if ( !kv ) return 0;


    int num = 0;

    KeyValues* pKey = kv->GetFirstSubKey();

    while ( pKey )
    {
        const char* name = pKey->GetName();
        const char* model = pKey->GetString( "model" );

        if ( *model )
        {
            list.AddToTail( new CZMPlayerModelData( pKey, bIsCustom ) );
            ++num;
        }
        else
        {
            Warning( "No model path for '%s'! Can't add as a player model!\n", name );
        }

        pKey = pKey->GetNextKey();
    }

    return num;
}

void CZMPlayerModelSystem::AddFallbackModel()
{
    KeyValues* kv = CZMPlayerModelData::CreateEmptyModelData( GetDefaultPlayerModel(), "_DefaultModel" );

    m_vPlayerModels.AddToTail( new CZMPlayerModelData( kv, false ) );

    kv->deleteThis();
}

CZMPlayerModelData* CZMPlayerModelSystem::GetRandomPlayerModel() const
{
    int count = m_vPlayerModels.Count();
    if ( !count )
    {
        return nullptr;
    }


    int index = random->RandomInt( 0, count - 1 );
    return m_vPlayerModels[index];
}

CZMPlayerModelData* CZMPlayerModelSystem::GetPlayerModelData( const char* model ) const
{
    int len = m_vPlayerModels.Count();
    for ( int i = 0; i < len; ++i )
    {
        if ( Q_strcmp( model, m_vPlayerModels[i]->GetModelName() ) == 0 )
        {
            return m_vPlayerModels[i];
        }
    }

    return nullptr;
}

int CZMPlayerModelSystem::FindPlayerModel( const char* model ) const
{
    int len = m_vPlayerModels.Count();
    for ( int i = 0; i < len; ++i )
    {
        if ( Q_stricmp( m_vPlayerModels[i]->GetModelName(), model ) == 0 )
        {
            return i;
        }
    }

    return -1;
}
//

CZMPlayerModelSystem* ZMGetPlayerModels()
{
    static CZMPlayerModelSystem s_ZMPlayerModels;
    return &s_ZMPlayerModels;
}

