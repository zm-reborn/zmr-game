#include "cbase.h"


#include "zmr_bitmapbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_BUILD_FACTORY( CZMBitMapButton );

CZMBitMapButton::CZMBitMapButton( vgui::Panel* parent, const char* name, const char* txt ) : CBitmapButton( parent, name, txt )
{
    m_pszTooltipName = nullptr;
}

CZMBitMapButton::CZMBitMapButton( vgui::Panel* parent, const char* name ) : CBitmapButton( parent, name, "" )
{
    m_pszTooltipName = nullptr;
}

CZMBitMapButton::~CZMBitMapButton()
{
    delete[] m_pszTooltipName;
    m_pszTooltipName = nullptr;
}

void CZMBitMapButton::OnCursorEntered()
{
    CBitmapButton::OnCursorEntered();


    if ( !m_pszTooltipName ) return;

    if ( !IsEnabled() ) return;


    m_iTooltipIndex = ZMClientUtil::ShowTooltipByName( m_pszTooltipName );
}
    
void CZMBitMapButton::OnCursorExited()
{
    CBitmapButton::OnCursorExited();


    if ( m_iTooltipIndex )
    {
        ZMClientUtil::HideTooltip( m_iTooltipIndex );
    }
}

const char* CZMBitMapButton::GetTooltipName() const
{
    return m_pszTooltipName;
}

void CZMBitMapButton::SetTooltipName( const char* msg )
{
    delete[] m_pszTooltipName;
    m_pszTooltipName = nullptr;

    if ( !msg ) return;


    int len = strlen( msg ) + 1;
    m_pszTooltipName = new char[len];
    Q_strncpy( m_pszTooltipName, msg, len );
}

// Stops the awful depressed offset shit.
void CZMBitMapButton::RecalculateDepressedState()
{
}

BitmapImage* CZMBitMapButton::GetBitmapImage( int index )
{
    return &(m_pImage[index]);
}
