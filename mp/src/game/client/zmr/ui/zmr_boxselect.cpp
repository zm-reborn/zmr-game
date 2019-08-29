#include "cbase.h"

#include "hud_macros.h"

#include "iclientmode.h"
//#include "in_buttons.h"
//#include "in_mouse.h"
#include "input.h"
#include "view.h"



#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>



#include "zmr/zmr_player_shared.h"

#include "zmr_boxselect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


CZMBoxSelect::CZMBoxSelect( Panel* pParent ) : Panel( g_pClientMode->GetViewport(), "ZMBoxSelect" )
{
    SetParent( pParent->GetVPanel() );

    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( false );
    SetKeyBoardInputEnabled( false );

    SetPos( 0, 0 );
    SetSize( ScreenWidth(), ScreenHeight() );
}

void CZMBoxSelect::Paint()
{
    int lefttop_x, lefttop_y;
    int rightbot_x, rightbot_y;


    if ( start_x < end_x )
    {
        lefttop_x = start_x;
        rightbot_x = end_x;
    }
    else
    {
        lefttop_x = end_x;
        rightbot_x = start_x;
    }


    if ( start_y < end_y )
    {
        lefttop_y = start_y;
        rightbot_y = end_y;
    }
    else
    {
        lefttop_y = end_y;
        rightbot_y = start_y;
    }


    surface()->DrawSetColor( 150, 0, 0, 40 );
    surface()->DrawFilledRect( lefttop_x, lefttop_y, rightbot_x, rightbot_y );
    surface()->DrawSetColor( 200, 0, 0, 150 );
    surface()->DrawOutlinedRect( lefttop_x, lefttop_y, rightbot_x, rightbot_y );
}

void CZMBoxSelect::SetStart( int x, int y )
{
    start_x = end_x = x;
    start_y = end_y = y;
}

void CZMBoxSelect::SetEnd( int x, int y )
{
    end_x = x;
    end_y = y;
}
