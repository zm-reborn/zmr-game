#include "cbase.h"

#include "zmr/c_zmr_player.h"
#include "zmr/c_zmr_util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CON_COMMAND( zm_hudchat, "Prints a message to chat." )
{
    C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();

    ZMClientUtil::PrintNotify( args.ArgS(), ( pLocal && pLocal->IsZM() ) ? ZMCHATNOTIFY_ZM : ZMCHATNOTIFY_NORMAL );
}
