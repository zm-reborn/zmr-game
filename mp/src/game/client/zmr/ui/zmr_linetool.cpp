#include "cbase.h"
#include "iclientmode.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>

#include "zmr_linetool.h"


using namespace vgui;


CZMLineTool::CZMLineTool( Panel* pParent ) : Panel( g_pClientMode->GetViewport(), "ZMLineTool" )
{
    SetParent( pParent->GetVPanel() );

    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( false );
    SetKeyBoardInputEnabled( false );

    SetPos( 0, 0 );
    SetSize( ScreenWidth(), ScreenHeight() );

    SetVisible( false );
}

void CZMLineTool::Paint()
{
    if ( !IsValidLine() ) return;


    // ZMRTODO: Get colors from scheme.
    surface()->DrawSetColor( 150, 0, 0, 255 );
    surface()->DrawLine( start_x, start_y, end_x, end_y );
}

void CZMLineTool::SetStart( int x, int y )
{
    start_x = end_x = x;
    start_y = end_y = y;
}

void CZMLineTool::SetEnd( int x, int y )
{
    end_x = x;
    end_y = y;
}

bool CZMLineTool::IsValidLine( int grace )
{
    return abs(end_x - start_x) > grace || abs(end_y - start_y) > grace;
}
