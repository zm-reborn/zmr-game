#include "cbase.h"

#include <vgui/ISurface.h>

#include "cdll_client_int.h"
#include "ienginevgui.h"

// :)
#include "../../../../gameui/optionsdialog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static vgui::DHANDLE<COptionsDialog> g_hGameUIOptions;

CON_COMMAND( OpenZMGameUIOptions, "" )
{
    if ( !g_hGameUIOptions.Get() )
    {
        vgui::VPANEL parent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
        if ( parent == NULL )
        {
            Assert( 0 );
            return;
        }

        auto* pPanel = new COptionsDialog( nullptr );
        pPanel->SetParent( parent );

        g_hGameUIOptions.Set( pPanel );
    }


    auto* pPanel = g_hGameUIOptions.Get();


    // Center
    int x, y, w, h;
    vgui::surface()->GetWorkspaceBounds( x, y, w, h );
    
    int mw = pPanel->GetWide();
    int mh = pPanel->GetTall();
    pPanel->SetPos( x + w / 2 - mw / 2, y + h / 2 - mh / 2 );


    pPanel->Activate();
}
