#pragma once


#include "steam/steam_api.h"

class CZMWeb
{
public:
    void QueryVersionNumber();


private:
    typedef CCallResult<CZMWeb, HTTPRequestCompleted_t> HTTPCallback;


    void Get( const char* url, HTTPCallback::func_t );

    HTTPCallback m_Callback;


    void Callback_Version( HTTPRequestCompleted_t*, bool );
    void ParseVersion( const char* );
};

extern CZMWeb* g_pZMWeb;
