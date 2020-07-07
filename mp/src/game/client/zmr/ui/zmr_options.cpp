#include "cbase.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"

#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/PropertySheet.h>

#include "c_zmr_player.h"
#include "zmr_options_general.h"
#include "zmr_options_graphics.h"
#include "zmr_options_misc.h"
#include "zmr_options_crosshair.h"
//#include "zmr_options_keys.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CZMOptionsMenu : public vgui::PropertyDialog
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsMenu, vgui::PropertyDialog );

    CZMOptionsMenu( vgui::VPANEL parent );
    ~CZMOptionsMenu();
};



static vgui::DHANDLE<CZMOptionsMenu> g_hZMOptions;

CON_COMMAND( OpenZMOptions, "" )
{
    if ( !g_hZMOptions.Get() )
    {
        vgui::VPANEL parent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
        if ( parent == NULL )
        {
            Assert( 0 );
            return;
        }

        g_hZMOptions.Set( new CZMOptionsMenu( parent ) );
    }


    auto* pPanel = g_hZMOptions.Get();


    // Center
    int x, y, w, h;
    vgui::surface()->GetWorkspaceBounds( x, y, w, h );
    
    int mw = pPanel->GetWide();
    int mh = pPanel->GetTall();
    pPanel->SetPos( x + w / 2 - mw / 2, y + h / 2 - mh / 2 );


    pPanel->Activate();
}


using namespace vgui;

CZMOptionsMenu::CZMOptionsMenu( VPANEL parent ) : BaseClass( nullptr, "ZMOptionsMenu" )
{
    SetParent( parent );


    SetBounds( 0, 0, 420, 350 );

	SetDeleteSelfOnClose( true );
    SetSizeable( false );

    SetTitle( "#GameUI_Options", true );


    AddPage( new CZMOptionsSubGeneral( this ), "General" );
    AddPage( new CZMOptionsSubGraphics( this ), "Video" );
    AddPage( new CZMOptionsSubCrosshair( this ), "Crosshair" );
    AddPage( new CZMOptionsSubMisc( this ), "Misc" );
    //AddPage( new CZMOptionsSubKeys( this ), "Keys" );
}

CZMOptionsMenu::~CZMOptionsMenu()
{
}
