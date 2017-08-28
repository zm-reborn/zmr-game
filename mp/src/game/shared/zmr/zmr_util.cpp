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
#endif
