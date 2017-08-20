#include "cbase.h"

#include "zmr/c_zmr_player.h"
#include "zmr_newversion.h"


using namespace vgui;



class CZMNewVerMenuInterface : public IZMUi
{
public:
    CZMNewVerMenuInterface() { m_Panel = nullptr; };

    void Create( VPANEL parent ) OVERRIDE
    {
        m_Panel = new CZMNewVerMenu( parent );
    }
    void Destroy() OVERRIDE
    {
        if ( m_Panel )
        {
            m_Panel->SetParent( nullptr );
            delete m_Panel;
        }
    }
    Panel* GetPanel() OVERRIDE { return m_Panel; }

private:
    CZMNewVerMenu* m_Panel;
};

static CZMNewVerMenuInterface g_ZMNewVerMenuInt;
IZMUi* g_pZMNewVerMenuInt = (IZMUi*)&g_ZMNewVerMenuInt;


CZMNewVerMenu::CZMNewVerMenu( VPANEL parent ) : BaseClass( nullptr, "ZMNewVerMenu" )
{
    g_pZMNewVerMenu = this;


    SetParent( parent );

    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( true );
    SetProportional( false );
    SetVisible( false );
    SetMoveable( true );
    SetSizeable( false );


    SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );


    LoadControlSettings( "resource/ui/zmnewver.res" );
}

CZMNewVerMenu::~CZMNewVerMenu()
{
}

CZMNewVerMenu* g_pZMNewVerMenu = nullptr;
