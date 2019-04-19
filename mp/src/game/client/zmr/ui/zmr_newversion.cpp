#include "cbase.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>

#include "cdll_client_int.h"
#include "ienginevgui.h"


#include "zmr/c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CZMNewVerMenu : public vgui::Frame
{
public:
    DECLARE_CLASS_SIMPLE( CZMNewVerMenu, vgui::Frame );

    CZMNewVerMenu( vgui::VPANEL parent );
    ~CZMNewVerMenu();
};


static vgui::DHANDLE<CZMNewVerMenu> g_hZMNewVer;

CON_COMMAND( OpenZMNewVersion, "" )
{
    if ( !g_hZMNewVer.Get() )
    {
        vgui::VPANEL parent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
        if ( parent == NULL )
        {
            Assert( 0 );
            return;
        }

        g_hZMNewVer.Set( new CZMNewVerMenu( parent ) );
    }


    auto* pPanel = g_hZMNewVer.Get();


    // Center
    int x, y, w, h;
    vgui::surface()->GetWorkspaceBounds( x, y, w, h );
    
    int mw = pPanel->GetWide();
    int mh = pPanel->GetTall();
    pPanel->SetPos( x + w / 2 - mw / 2, y + h / 2 - mh / 2 );


    pPanel->Activate();
}


CZMNewVerMenu::CZMNewVerMenu( vgui::VPANEL parent ) : BaseClass( nullptr, "ZMNewVerMenu" )
{
    SetParent( parent );

    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    SetProportional( false );
    SetDeleteSelfOnClose( true );
    SetMoveable( true );
    SetSizeable( false );


    //SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );


    LoadControlSettings( "resource/ui/zmnewver.res" );
}

CZMNewVerMenu::~CZMNewVerMenu()
{
}
