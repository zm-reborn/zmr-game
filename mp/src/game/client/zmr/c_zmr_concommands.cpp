#include "cbase.h"

#include "zmr/zmr_util.h"



CON_COMMAND( zm_hudchat, "Prints a message to chat." )
{
    ZMClientUtil::PrintNotify( args.ArgS() );
}
