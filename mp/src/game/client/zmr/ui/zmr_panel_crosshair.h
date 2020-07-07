#pragma once

#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
//#include "GameEventListener.h"
//#include "KeyValues.h"

#include "c_zmr_crosshair.h"


class CZMPanelCrosshair : public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMPanelCrosshair, vgui::EditablePanel );


    CZMPanelCrosshair( vgui::Panel* parent, const char* name );
    ~CZMPanelCrosshair();


    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

    virtual void Paint();


    void SetCrosshairToDraw( CZMBaseCrosshair* pCross );


protected:
    CZMBaseCrosshair* m_pDrawCrosshair;
};