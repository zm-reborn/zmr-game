#include "cbase.h"
#include "vgui_controls/PropertyDialog.h"

#include <vgui/IVGui.h>
#include <vgui_controls/AnimationController.h>

#include "zmr_options.h"
#include "zmr/c_zmr_player.h"
#include "zmr_options_general.h"
#include "zmr_options_graphics.h"





using namespace vgui;

class CZMOptionsMenu : public PropertyDialog
{
public:
    DECLARE_CLASS_SIMPLE( CZMOptionsMenu, PropertyDialog );

    CZMOptionsMenu( VPANEL parent );
    ~CZMOptionsMenu();
    
    
    virtual void OnTick() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    
    virtual void OnCommand( const char* ) OVERRIDE;
};

class CZMOptionsMenuInterface : public IZMUi
{
public:
    CZMOptionsMenuInterface() { m_Panel = nullptr; };

    void Create( VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMOptionsMenu( parent );
    }
    void Destroy() OVERRIDE
    {
        if ( m_Panel )
        {
            m_Panel->SetParent( nullptr );
            delete m_Panel;
        }
    }
    vgui::Panel* GetPanel() OVERRIDE { return m_Panel; }

private:
    CZMOptionsMenu* m_Panel;
};

static CZMOptionsMenuInterface g_ZMOptionsMenuInt;
IZMUi* g_pZMOptionsMenu = (IZMUi*)&g_ZMOptionsMenuInt;



static bool g_bZMOptionsShow = false;

CON_COMMAND( ToggleZMOptions, "" )
{
    g_bZMOptionsShow = !g_bZMOptionsShow;
}

CZMOptionsMenu::CZMOptionsMenu( VPANEL parent ) : BaseClass( nullptr, "ZMOptionsMenu" )
{
    SetParent( parent );

    //SetDeleteSelfOnClose( true );
    SetBounds( 150, 100, 420, 350 );

    SetVisible( false );
    SetSizeable( false );

    SetTitle( "#GameUI_Options", true );


    AddPage( new CZMOptionsSubGeneral( this ), "General" );
    AddPage( new CZMOptionsSubGraphics( this ), "Video" );


    vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CZMOptionsMenu::~CZMOptionsMenu()
{
}

void CZMOptionsMenu::OnTick()
{
    BaseClass::OnTick();

    
    if ( g_bZMOptionsShow != IsVisible() )
    {
        SetVisible( g_bZMOptionsShow );

        if ( IsVisible() )
            //UpdateMenu();

        return;
    }
}

void CZMOptionsMenu::OnThink()
{
    
}

void CZMOptionsMenu::OnCommand( const char* command )
{
    if ( Q_stricmp( command, "Close" ) == 0 )
    {
        // Don't do fade in/our effects right now.
        g_bZMOptionsShow = false;
        return;
    }
    else if ( Q_stricmp( command, "Ok" ) == 0 )
    {
        //ApplySettings();

        g_bZMOptionsShow = false;
    }

    BaseClass::OnCommand( command );
}
