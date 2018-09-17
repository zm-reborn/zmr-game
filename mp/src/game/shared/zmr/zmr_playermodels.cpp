#include "cbase.h"
#include "filesystem.h"


#include "zmr_playermodels.h"



#define PLAYERMODEL_FILE            "resource/zmplayermodels.txt"

#define DEF_PLAYER_MODEL            "models/male_pi.mdl"



//
CZMPlayerModelData::CZMPlayerModelData( KeyValues* kv, bool bIsCustom )
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
//


//
CZMPlayerModelSystem::CZMPlayerModelSystem()
{

}

CZMPlayerModelSystem::~CZMPlayerModelSystem()
{
    m_vPlayerModels.PurgeAndDeleteElements();
}

const char* CZMPlayerModelSystem::GetDefaultPlayerModel()
{
    return DEF_PLAYER_MODEL;
}

int CZMPlayerModelSystem::LoadModelsFromFile()
{
#ifdef CLIENT_DLL
    if ( m_vPlayerModels.Count() )
        return m_vPlayerModels.Count();
#endif

    KeyValues* kv = new KeyValues( "PlayerModels" );
    if ( !kv->LoadFromFile( filesystem, PLAYERMODEL_FILE ) )
    {
        kv->deleteThis();
        return m_vPlayerModels.Count();
    }



    m_vPlayerModels.PurgeAndDeleteElements();


    LoadModelData( kv );
    kv->deleteThis();

    // We need to add something for the server.
#ifdef GAME_DLL
    if ( !m_vPlayerModels.Count() )
    {
        AddDefaultPlayerModels();
    }
#endif

    return m_vPlayerModels.Count();
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
            const char* arms = pData->GetModelData()->GetString( "handsmodel" );
            if ( *arms && CBaseEntity::PrecacheModel( model ) == -1 )
            {
                pData->GetModelData()->SetString( "handsmodel", "" );

                Warning( "Invalid arm model path '%s' ('%s')!\n", arms, name );
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

int CZMPlayerModelSystem::LoadModelData( KeyValues* kv )
{
    if ( !kv ) return 0;


    KeyValues* pKey = kv->GetFirstSubKey();

    while ( pKey )
    {
        const char* name = pKey->GetName();
        const char* model = pKey->GetString( "model" );

        if ( *model )
        {
            m_vPlayerModels.AddToTail( new CZMPlayerModelData( pKey ) );
        }
        else
        {
            Warning( "No model path for '%s'! Can't add as a player model!\n", name );
        }

        pKey = pKey->GetNextKey();
    }

    return m_vPlayerModels.Count();
}

void CZMPlayerModelSystem::AddDefaultPlayerModels()
{
    KeyValues* pNewKey = new KeyValues( "" );

    KeyValues* pSub = pNewKey->FindKey( "_DefaultModel", true );
    pSub->SetString( "model", GetDefaultPlayerModel() );

    m_vPlayerModels.AddToTail( new CZMPlayerModelData( pSub, false ) );

    pNewKey->deleteThis();
}


CZMPlayerModelData* CZMPlayerModelSystem::GetPlayerModelData( const char* model )
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

int CZMPlayerModelSystem::FindPlayerModel( const char* model )
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

