#include "cbase.h"

#ifdef CLIENT_DLL
#include "view_scene.h"
#endif

#include "zmr/zmr_global_shared.h"
#include "zmr_util.h"

#ifdef CLIENT_DLL
bool ZMClientUtil::WorldToScreen( const Vector& pos, Vector& screen, int& x, int& y )
{
    int behind = ScreenTransform( pos, screen );

    if ( !behind )
    {
        int w = ScreenWidth();
        int h = ScreenHeight();
        x =  0.5f * screen[0] * w;
        y = -0.5f * screen[1] * h;
        x += 0.5f * w;
        y += 0.5f * h;
    }

    return !behind ? true : false;
}

int ZMClientUtil::GetSelectedZombieCount()
{
    int myindex = GetLocalPlayerIndex();

    if ( !myindex ) return 0;


    int num = 0;

    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        if ( g_pZombies->Element( i )->GetSelectorIndex() == myindex )
        {
            ++num;
        }
    }

    return num;
}

void ZMClientUtil::SelectAllZombies( bool bSendCommand )
{
    int index = GetLocalPlayerIndex();
    if ( !index ) return;


    C_ZMBaseZombie* pZombie;

    int len = g_pZombies->Count();
    for ( int i = 0; i < len; i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie )
        {
            pZombie->SetSelector( index );
        }
    }


    if ( bSendCommand )
    {
        // Tell server we're selecting all.
        engine->ClientCmd( "zm_cmd_selectall" );
    }
}

void ZMClientUtil::DeselectAllZombies( bool bSendCommand )
{
    int index = GetLocalPlayerIndex();
    if ( !index ) return;


    C_ZMBaseZombie* pZombie;

    int len = g_pZombies->Count();
    for ( int i = 0; i < len; i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelectorIndex() == index )
        {
            pZombie->SetSelector( 0 );
        }
    }


    if ( bSendCommand )
    {
        // Tell server we're deselecting.
        engine->ClientCmd( "zm_cmd_unselectall" );
    }
}

void ZMClientUtil::SelectSingleZombie( C_ZMBaseZombie* pZombie, bool bSticky )
{
    if ( !bSticky )
    {
        DeselectAllZombies( false );
    }

    int index = GetLocalPlayerIndex();

    engine->ClientCmd( VarArgs( "zm_cmd_select %i%s",
        pZombie->entindex(),
        bSticky ? " 1" : "" ) );

    pZombie->SetSelector( index );
}

void ZMClientUtil::SelectZombies( const CUtlVector<C_ZMBaseZombie*>& vZombies, bool bSticky )
{
    C_ZMBaseZombie* pZombie;
    int index = GetLocalPlayerIndex();


    if ( !index ) return;

    if ( !bSticky )
    {
        // Send command if we didn't select anything.
        DeselectAllZombies( !vZombies.Count() );
    }


    // We didn't select anything.
    if ( !vZombies.Count() )
    {
        return;
    }

    char cmdbuffer[512];
    cmdbuffer[0] = 0;

    for ( int i = 0; i < vZombies.Count(); i++ )
    {
        pZombie = vZombies.Element( i );

        pZombie->SetSelector( index );

        Q_snprintf( cmdbuffer, sizeof( cmdbuffer ), "%s%i ", cmdbuffer, pZombie->entindex() );
    }

    if ( cmdbuffer[0] )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_selectmult %s %s",  bSticky ? "1": "0", cmdbuffer ) );
    }
}
#endif
