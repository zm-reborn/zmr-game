#pragma once


#include <vgui_controls/Panel.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

// Acts like a frame but without all the useless elements.
class CZMFramePanel : public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMFramePanel, vgui::EditablePanel );

    CZMFramePanel( vgui::Panel* pParent, const char* szName = "" ) : vgui::EditablePanel( pParent, szName )
    {
        MakePopup( false );
        GetFocusNavGroup().SetFocusTopLevel( true );
    }
    
    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE
    {
        if ( !IsBuildGroupEnabled() )
        {
            // if a child doesn't have focus, get it for ourselves
            VPANEL focus = vgui::input()->GetFocus();
            if ( !focus || !ipanel()->HasParent( focus, GetVPanel() ) )
            {
                RequestFocus();
            }
        }
    
        BaseClass::OnMousePressed( code );
    }
};
