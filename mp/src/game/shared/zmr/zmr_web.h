#pragma once


#include "steam/steam_api.h"

class CZMWeb
{
public:
    void QueryVersionNumber();


private:
    typedef CCallResult<CZMWeb, HTTPRequestCompleted_t> HTTPCallback;


    void Get( const char* url, HTTPCallback::func_t func );

    void Callback_Version( HTTPRequestCompleted_t* pResult, bool bIOFailure );
    void ParseVersion( const char* pszVersionString );


    HTTPCallback m_Callback;
};

extern CZMWeb* g_pZMWeb;
