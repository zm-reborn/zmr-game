#include "cbase.h"

#include "zmr/zmr_global_shared.h"
#include "zmr_util.h"


int ZMUtil::GetSelectedZombieCount( int iPlayerIndex )
{
    int num = 0;

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( !pZombie ) continue;


        if ( pZombie->GetSelectorIndex() == iPlayerIndex )
        {
            ++num;
        }
    }

    return num;
}

void ZMUtil::PrintNotify( CBasePlayer* pPlayer, ZMChatNotifyType_t type, const char* msg )
{
    CSingleUserRecipientFilter filter( pPlayer );
    filter.MakeReliable();

    SendNotify( filter, type, msg );
}

void ZMUtil::PrintNotifyAll( ZMChatNotifyType_t type, const char* msg )
{
    CRecipientFilter filter;
    filter.AddAllPlayers();
    filter.MakeReliable();

    SendNotify( filter, type, msg );
}

void ZMUtil::SendNotify( IRecipientFilter& filter, ZMChatNotifyType_t type, const char* msg )
{
    if ( filter.GetRecipientCount() < 1 )
        return;


    UserMessageBegin( filter, "ZMChatNotify" );
        WRITE_BYTE( type );
        WRITE_STRING( msg );
    MessageEnd();
}
