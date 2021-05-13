#pragma once

#include "vgui_bitmapbutton.h"

#include "c_zmr_util.h"
#include "zmr_hud_tooltips.h"


class CZMBitMapButton : public CBitmapButton
{
public:
    CZMBitMapButton( vgui::Panel* parent, const char* name, const char* txt );
    CZMBitMapButton( vgui::Panel* parent, const char* name );

    ~CZMBitMapButton();

    virtual void OnCursorEntered() OVERRIDE;
    
    virtual void OnCursorExited() OVERRIDE;

    const char* GetTooltipName() const;
    void SetTooltipName( const char* msg );

    // Stops the awful depressed offset shit.
    virtual void RecalculateDepressedState() OVERRIDE;

    BitmapImage* GetBitmapImage( int index );

private:
    char* m_pszTooltipName;
    int m_iTooltipIndex;
};
