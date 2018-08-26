#include "cbase.h"
#include <vgui/IInput.h>
//#include <vgui_controls/Controls.h>


#include "zmr_framepanel.h"


CZMFramePanel::CZMFramePanel( vgui::Panel* pParent, const char* szName ) : vgui::EditablePanel( pParent, szName )
{
    MakePopup( false );
    GetFocusNavGroup().SetFocusTopLevel( true );
}
    
void CZMFramePanel::OnMousePressed( vgui::MouseCode code )
{
    if ( !IsBuildGroupEnabled() )
    {
        // If a child doesn't have focus, get it for ourselves
        vgui::VPANEL focus = vgui::input()->GetFocus();
        if ( !focus || !vgui::ipanel()->HasParent( focus, GetVPanel() ) )
        {
            RequestFocus();
        }
    }
    
    BaseClass::OnMousePressed( code );
}
