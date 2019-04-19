#include "cbase.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/RichText.h>

#include "cdll_client_int.h"
#include "filesystem.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CZMCreditsMenu : public vgui::Frame
{
public:
    DECLARE_CLASS_SIMPLE( CZMCreditsMenu, vgui::Frame );

    CZMCreditsMenu( vgui::VPANEL parent );
    ~CZMCreditsMenu();

private:
    vgui::RichText* m_pRichText;
};


static vgui::DHANDLE<CZMCreditsMenu> g_hCreditsMenu;

CON_COMMAND( OpenZMCreditsMenu, "" )
{
    if ( !g_hCreditsMenu.Get() )
    {
        vgui::VPANEL parent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
        if ( parent == NULL )
        {
            Assert( 0 );
            return;
        }

        auto* pPanel = new CZMCreditsMenu( parent );

        g_hCreditsMenu.Set( pPanel );
    }


    auto* pPanel = g_hCreditsMenu.Get();


    // Center
    int x, y, w, h;
    vgui::surface()->GetWorkspaceBounds( x, y, w, h );
    
    int mw = pPanel->GetWide();
    int mh = pPanel->GetTall();
    pPanel->SetPos( x + w / 2 - mw / 2, y + h / 2 - mh / 2 );


    pPanel->Activate();
}

CZMCreditsMenu::CZMCreditsMenu( vgui::VPANEL parent ) : BaseClass( nullptr, "ZMCreditsMenu" )
{
    SetParent( parent );

    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    SetProportional( false );
    SetDeleteSelfOnClose( true );
    SetMoveable( true );
    SetSizeable( false );


    m_pRichText = new vgui::RichText( this, "RichText1" );


    LoadControlSettings( "resource/ui/zmcredits.res" );


    auto hndl = filesystem->Open( "resource/zmr_credits.txt", "rb", "MOD" );
    if ( hndl )
    {
        unsigned int size = filesystem->Size( hndl );

        char* buf = new char[size];
        buf[size-1] = NULL;

        filesystem->Read( buf, size-1, hndl );

        m_pRichText->InsertString( buf );

        delete[] buf;


        filesystem->Close( hndl );
    }
}

CZMCreditsMenu::~CZMCreditsMenu()
{
}
