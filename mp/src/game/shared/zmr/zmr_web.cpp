#include "cbase.h"

#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_web.h"


#define VERSION_URL     "https://raw.githubusercontent.com/zm-reborn/zmr-game/master/version.txt"




ConVar zm_checkversion( "zm_checkversion", "1" );

void CZMWeb::Get( const char* url, HTTPCallback::func_t func )
{
    if ( !steamapicontext || !steamapicontext->SteamHTTP() ) return;

    
    HTTPRequestHandle req = steamapicontext->SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, url );


    SteamAPICall_t call;

    if ( steamapicontext->SteamHTTP()->SendHTTPRequest( req, &call ) )
    {
        m_Callback.Set( call, this, func );
    }
    else
    {
        steamapicontext->SteamHTTP()->ReleaseHTTPRequest( req );
    }
}

void CZMWeb::QueryVersionNumber()
{
    if ( zm_checkversion.GetBool() )
    {
        Get( VERSION_URL, &CZMWeb::Callback_Version );
    }
}

void CZMWeb::Callback_Version( HTTPRequestCompleted_t* pResult, bool bIOFailure )
{
    if ( !steamapicontext || !steamapicontext->SteamHTTP() ) return;

    if ( bIOFailure ) return;

    if ( !pResult || !pResult->m_bRequestSuccessful ) return;

    if ( pResult->m_eStatusCode == k_EHTTPStatusCode502BadGateway ) return;


    uint32 size;
    steamapicontext->SteamHTTP()->GetHTTPResponseBodySize( pResult->m_hRequest, &size );

    if ( !size ) return;


    uint8* data = new uint8[size];
    steamapicontext->SteamHTTP()->GetHTTPResponseBodyData( pResult->m_hRequest, data, size );


    ParseVersion( reinterpret_cast<char*>( data ) );

    delete[] data;
    steamapicontext->SteamHTTP()->ReleaseHTTPRequest( pResult->m_hRequest );
}

void CZMWeb::ParseVersion( const char* data )
{
    const char* sep[] = { ".", "\n" };

    CSplitString strs( data, sep, ARRAYSIZE( sep ) );


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
            Msg( "New version of %s is available!\n", ZMR_NAME );
            return;
        }
    }
}

static CZMWeb g_ZMWeb;
CZMWeb* g_pZMWeb = &g_ZMWeb;
