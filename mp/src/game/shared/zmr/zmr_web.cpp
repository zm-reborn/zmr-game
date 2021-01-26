#include "cbase.h"

#include "zmr_shareddefs.h"
#include "zmr_web.h"

#ifndef CLIENT_DLL
#include "steam/steam_gameserver.h"

#define STEAM_API     steamgameserverapicontext
#else
#define STEAM_API     steamapicontext
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define VERSION_URL     "http://version.zmreborn.com"


void CZMWeb::Get( const char* url, HTTPCallback::func_t func )
{
    if ( !STEAM_API || !STEAM_API->SteamHTTP() )
    {
        Assert( 0 );
        return;
    }
    
    HTTPRequestHandle req = STEAM_API->SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, url );


    SteamAPICall_t call;

    if ( STEAM_API->SteamHTTP()->SendHTTPRequest( req, &call ) )
    {
        m_Callback.Set( call, this, func );
    }
    else
    {
        STEAM_API->SteamHTTP()->ReleaseHTTPRequest( req );
    }
}

void CZMWeb::QueryVersionNumber()
{
    DevMsg( "Querying version...\n" );

    Get( VERSION_URL, &CZMWeb::Callback_Version );
}

void CZMWeb::Callback_Version( HTTPRequestCompleted_t* pResult, bool bIOFailure )
{
    if ( !STEAM_API || !STEAM_API->SteamHTTP() )
    {
        Assert( 0 );
        return;
    }

    if ( !pResult || !pResult->m_hRequest ) return;


    if (!bIOFailure &&
        pResult->m_bRequestSuccessful &&
        pResult->m_eStatusCode == k_EHTTPStatusCode200OK &&
        pResult->m_unBodySize > 0 )
    {
        uint8* data = new uint8[pResult->m_unBodySize + 1];
        STEAM_API->SteamHTTP()->GetHTTPResponseBodyData( pResult->m_hRequest, data, pResult->m_unBodySize );
        data[pResult->m_unBodySize] = NULL;

        ParseVersion( reinterpret_cast<char*>( data ) );

        delete[] data;
    }

    STEAM_API->SteamHTTP()->ReleaseHTTPRequest( pResult->m_hRequest );
}

void CZMWeb::ParseVersion( const char* pszVersionString )
{
    // Typical version string should be
    // cXX.sXX
    // Where
    // cXX = client version
    // sXX = server version
    //
    if ( pszVersionString[0] != 'c' && pszVersionString[0] != 's' )
    {
        Assert( 0 );
        return;
    }


    const char* sep[] = { ".", "\n" };

    CSplitString strs( pszVersionString, sep, ARRAYSIZE( sep ) );


    int len = strs.Count() >= 2 ? 2 : 0;
    for ( int i = 0; i < len; i++ )
    {
#ifdef CLIENT_DLL
        if ( strs[i][0] != 'c' )
#else
        if ( strs[i][0] != 's' )
#endif
            continue;


        if ( !FStrEq( &(strs[i][1]), ZMR_VERSION ) )
        {
#ifdef CLIENT_DLL
            engine->ClientCmd_Unrestricted( "OpenZMNewVersion" );
#else
            UTIL_LogPrintf( "New version of %s is available!\n", ZMR_NAME );
#endif

            Msg( "*\n*\n* New version of %s is available!\n*\n*\n", ZMR_NAME );

            DevMsg( "Version string: %s\n", pszVersionString );

            return;
        }
    }
}

static CZMWeb g_ZMWeb;
CZMWeb* g_pZMWeb = &g_ZMWeb;
