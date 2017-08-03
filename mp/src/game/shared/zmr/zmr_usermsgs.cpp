#include "cbase.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


/*
    NOTE: Add this function at the end of hl2/hl2_usermessages.cpp

*/

void RegisterZMUserMessages( void )
{
    usermessages->Register( "ZMBuildMenuUpdate", -1 ); // 1 * spawn ehandle, 5 * queue bytes
    usermessages->Register( "ZMManiMenuUpdate", -1 );
}