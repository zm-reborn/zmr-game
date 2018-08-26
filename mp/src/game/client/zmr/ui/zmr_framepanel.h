#pragma once


#include <vgui_controls/EditablePanel.h>

// Acts like a frame but without all the useless elements.
class CZMFramePanel : public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMFramePanel, vgui::EditablePanel );

    CZMFramePanel( vgui::Panel* pParent, const char* szName = "" );
    
    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
};
