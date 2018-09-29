#include "cbase.h"


#include "zmr_mainmenu_basebtn.h"



#include "cbase.h"





#include "zmr_mainmenu.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu_subbtn.h"


using namespace vgui;



CZMMainMenuBaseButton::CZMMainMenuBaseButton( Panel* pParent, const char* name ) : BaseClass( pParent, name, "" )
{
    SetPaintBackgroundEnabled( false );
    SetPaintBorderEnabled( false );
}

CZMMainMenuBaseButton::~CZMMainMenuBaseButton()
{
}

void CZMMainMenuBaseButton::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );


    Color empty( 0, 0, 0, 0 );

    SetDefaultColor( COLOR_WHITE, empty );
    SetArmedColor( Color( 255, 255, 200, 255 ), empty );
}

CZMMainMenu* CZMMainMenuBaseButton::GetMainMenu()
{
    return static_cast<CZMMainMenu*>( GetParent() );
}
