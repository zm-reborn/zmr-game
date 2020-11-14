#include "cbase.h"
#include "filesystem.h"
#include "c_playerresource.h"

#include <steam/steam_api.h>

#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>


#undef min
#undef max
#include <algorithm>


#include "c_zmr_importancesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void C_ZMImportanceSystem::ImportanceData_t::Init( int uid )
{
    userId = uid;
    importance = ZMIMPORTANCE_NONE;
}

bool C_ZMImportanceSystem::ImportanceData_t::IsValid( int playerIndex ) const
{
    if ( userId == -1 )
        return false;


    player_info_t pi;
    if ( !engine->GetPlayerInfo( playerIndex, &pi ) )
    {
        return false;
    }

    return pi.userID == userId;
}

C_ZMImportanceSystem::C_ZMImportanceSystem()
{
    Reset();
}

C_ZMImportanceSystem::~C_ZMImportanceSystem()
{
}

void C_ZMImportanceSystem::PostInit()
{
    m_vPlayerData.RemoveAll();
    m_vSteamIdIndices.RemoveAll();


    LoadFromFile();
}

void C_ZMImportanceSystem::LevelInitPostEntity()
{
    Reset();
}

bool C_ZMImportanceSystem::LoadFromFile()
{
    auto* kv = new KeyValues( "ImportantPeople" );
    if ( !kv->LoadFromFile( filesystem, "resource/zmimportantpeople.txt" ) )
    {
        return false;
    }



    // Load the steam ids.
    for ( auto* data = kv->GetFirstSubKey(); data; data = data->GetNextKey() )
    {
        const char* steamId = data->GetName();

        m_vSteamIdIndices.AddToTail( Q_atoui64( steamId ) );
    }
    

    std::sort(
        m_vSteamIdIndices.begin(),
        m_vSteamIdIndices.end(),
        []( const uint64& s0, const uint64& s1 ) {
            return s0 < s1;
        } );



    m_vPlayerData.EnsureCount( m_vSteamIdIndices.Count() );


    // Load the importance.
    // This is probably not the best method...
    for ( auto* data = kv->GetFirstSubKey(); data; data = data->GetNextKey() )
    {
        const char* steamId = data->GetName();


        int i = FindSteamIdIndex( Q_atoui64( steamId ) );
        Assert( i != -1 );

        if ( i == -1 )
            continue;


        int importance = data->GetInt( nullptr, -1 );

        m_vPlayerData[i] = (ZMImportance_t)importance;
    }


    kv->deleteThis();

    return true;
}

int C_ZMImportanceSystem::FindSteamIdIndex( uint64 steamId )
{
    auto* begin = m_vSteamIdIndices.begin();
    auto* end = m_vSteamIdIndices.end();


    // Binary search
    auto* found = std::lower_bound( begin, end, steamId );


    if ( found == end )
        return -1;

    if ( *found != steamId )
        return -1;

    return (int)(found - begin);
}

const char* C_ZMImportanceSystem::GetPlayerImportanceName( int playerIndex )
{
    if ( !IsCached( playerIndex ) )
    {
        ComputePlayerImportance( playerIndex );
    }

    return ImportanceToName( m_Importance[playerIndex].importance );
}

ZMImportance_t C_ZMImportanceSystem::GetPlayerImportance( int playerIndex )
{
    if ( !IsCached( playerIndex ) )
    {
        ComputePlayerImportance( playerIndex );
    }

    return m_Importance[playerIndex].importance;
}

bool C_ZMImportanceSystem::IsCached( int playerIndex )
{
    return m_Importance[playerIndex].IsValid( playerIndex );
}

void C_ZMImportanceSystem::Reset()
{
    for ( int i = 0; i < ARRAYSIZE( m_Importance ); i++ )
    {
        m_Importance[i].Init( -1 );
    }
}

const char* C_ZMImportanceSystem::ImportanceToName( ZMImportance_t index )
{
    switch ( index )
    {
    case ZMIMPORTANCE_DEV : return "Developer";
    case ZMIMPORTANCE_VIP : return "VIP";
    case ZMIMPORTANCE_PLAYTESTER : return "Playtester";
    default : return "";
    }
}

ZMImportance_t C_ZMImportanceSystem::ImportanceNameToIndex( const char* name )
{
    if ( Q_stricmp( "VIP", name ) == 0 )
    {
        return ZMIMPORTANCE_VIP;
    }

    if ( Q_stricmp( "Playtester", name ) == 0 )
    {
        return ZMIMPORTANCE_PLAYTESTER;
    }

    if ( Q_stricmp( "Developer", name ) == 0 )
    {
        return ZMIMPORTANCE_DEV;
    }

    return ZMIMPORTANCE_NONE;
}

bool C_ZMImportanceSystem::ComputePlayerImportance( int playerIndex )
{
    // Get Steam ID.
    Assert( steamapicontext && steamapicontext->SteamUtils() );
    if ( !steamapicontext || !steamapicontext->SteamUtils() )
    {
        return false;
    }


    player_info_t pi;
    if ( !engine->GetPlayerInfo( playerIndex, &pi ) )
    {
        return false;
    }

    if ( !pi.friendsID )
        return false;


    CSteamID id;

    {
        static EUniverse universe = k_EUniverseInvalid;

        if ( universe == k_EUniverseInvalid )
            universe = steamapicontext->SteamUtils()->GetConnectedUniverse();

        id.InstancedSet( pi.friendsID, 1, universe, k_EAccountTypeIndividual );
    }


    if ( !id.IsValid() )
        return false;



    m_Importance[playerIndex].Init( pi.userID );


    // Find a steam id and assign the importance.
    int i = FindSteamIdIndex( id.ConvertToUint64() );
    if ( i != -1 )
    {
        m_Importance[playerIndex].importance = m_vPlayerData[i];
    }


    return true;
}

C_ZMImportanceSystem g_ZMImportanceSystem;
