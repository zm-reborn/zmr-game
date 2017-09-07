#pragma once

#include "vgui_bitmapbutton.h"

#include "zmr/zmr_util.h"
#include "zmr_hud_tooltips.h"


class CZMBitMapButton : public CBitmapButton
{
public:
    CZMBitMapButton( vgui::Panel* parent, const char* name, const char* txt ) : CBitmapButton( parent, name, txt )
    {
        m_pszTooltipName = nullptr;
    }

    ~CZMBitMapButton()
    {
        delete[] m_pszTooltipName;
    }

    virtual void OnCursorEntered() OVERRIDE
    {
        CBitmapButton::OnCursorEntered();


        if ( !m_pszTooltipName ) return;

        if ( !IsEnabled() ) return;


        m_iTooltipIndex = ZMClientUtil::ShowTooltipByName( m_pszTooltipName );
    }

    virtual void OnCursorExited() OVERRIDE
    {
        CBitmapButton::OnCursorExited();


        if ( m_iTooltipIndex )
        {
            ZMClientUtil::HideTooltip( m_iTooltipIndex );
        }
    }

    const char* GetTooltipName() { return m_pszTooltipName; };

    void SetTooltipName( const char* msg )
    {
        delete[] m_pszTooltipName;

        if ( !msg ) return;


        int len = strlen( msg ) + 1;
        m_pszTooltipName = new char[len];
        Q_strncpy( m_pszTooltipName, msg, len );
    }

private:
    char* m_pszTooltipName;
    int m_iTooltipIndex;
};
