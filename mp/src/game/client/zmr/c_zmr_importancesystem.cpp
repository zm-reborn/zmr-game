#include "cbase.h"
#include "filesystem.h"

#include <steam/steam_api.h>

#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>

#include "c_zmr_importancesystem.h"


C_ZMImportanceSystem::C_ZMImportanceSystem()
{
    m_pImageTrusted = nullptr;
    m_pImagePlaytester = nullptr;

    ResetCached();
}

C_ZMImportanceSystem::~C_ZMImportanceSystem()
{
}

void C_ZMImportanceSystem::InitImages()
{
    m_pImageTrusted = vgui::scheme()->GetImage( "zmr_misc/trusted", true );
    m_pImagePlaytester = vgui::scheme()->GetImage( "zmr_misc/playtester", true );
}

vgui::IImage* C_ZMImportanceSystem::GetPlayerImportanceImageIndex( int playerIndex )
{
    if ( m_iCachedImportance[playerIndex] == -1 )
    {
        m_iCachedImportance[playerIndex] = ComputePlayerImportance( playerIndex );
    }

    return ImportanceIndexToImage( m_iCachedImportance[playerIndex] );
}

void C_ZMImportanceSystem::ResetCached()
{
    for ( int i = 0; i < ARRAYSIZE( m_iCachedImportance ); i++ )
    {
        m_iCachedImportance[i] = -1;
    }
}

vgui::IImage* C_ZMImportanceSystem::ImportanceIndexToImage( int index )
{
    switch ( index )
    {
    case 1 : return m_pImageTrusted;
    case 2 : return m_pImagePlaytester;
    default : return nullptr;
    }
}

int C_ZMImportanceSystem::ImportanceNameToIndex( const char* name )
{
    if ( Q_stricmp( "Trusted", name ) == 0 )
    {
        return 1;
    }

    if ( Q_stricmp( "Playtester", name ) == 0 )
    {
        return 2;
    }

    return -1;
}

int C_ZMImportanceSystem::ComputePlayerImportance( int playerIndex )
{
    //
    // Get Steam ID.
    //
    Assert( steamapicontext && steamapicontext->SteamUtils() );
    if ( !steamapicontext || !steamapicontext->SteamUtils() )
    {
        return -1;
    }


    player_info_t pi;
    if ( !engine->GetPlayerInfo( playerIndex, &pi ) )
    {
        return -1;
    }

    if ( !pi.friendsID )
        return -1;


    CSteamID id;

    {
        static EUniverse universe = k_EUniverseInvalid;

        if ( universe == k_EUniverseInvalid )
            universe = steamapicontext->SteamUtils()->GetConnectedUniverse();

        id.InstancedSet( pi.friendsID, 1, universe, k_EAccountTypeIndividual );
    }


    if ( !id.IsValid() )
        return -1;



    auto* kv = new KeyValues( "ImportantPeople" );
    if ( !kv->LoadFromFile( filesystem, "resource/zmimportantpeople.txt" ) )
    {
        return -1;
    }

    for ( auto* sub = kv->GetFirstTrueSubKey(); sub; sub = sub->GetNextTrueSubKey() )
    {
        int index = ImportanceNameToIndex( sub->GetName() );

        for ( auto* data = sub->GetFirstSubKey(); data; data = data->GetNextKey() )
        {
            const char* steamId = data->GetName();
            CSteamID cid;
            cid.SetFromUint64( Q_atoui64( steamId ) );

            if ( cid.IsValid() && cid == id )
            {
                return index;
            }
        }
    }


    return -1;
}

C_ZMImportanceSystem g_ZMImportanceSystem;
