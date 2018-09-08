#include "cbase.h"

#include "zmr_zmview_old.h"




ConVar zm_cl_usenewmenus( "zm_cl_usenewmenus", "1", FCVAR_ARCHIVE, "Use new ZM menus?" );


CZMViewOld::CZMViewOld( const char* pElementName ) : CZMViewBase( pElementName )
{
	m_pZMControl = new CZMHudControlPanel( this );

	m_pManiMenu = new CZMManiMenu( this ); 
	m_pManiMenuNew = new CZMManiMenuNew( this ); 
	m_pBuildMenu = new CZMBuildMenu( this ); 
	m_pBuildMenuNew = new CZMBuildMenuNew( this ); 
}

CZMViewOld::~CZMViewOld()
{
    delete m_pZMControl;

    delete m_pManiMenu;
    delete m_pManiMenuNew;
    delete m_pBuildMenu;
    delete m_pBuildMenuNew;
}

CZMBuildMenuBase* CZMViewOld::GetBuildMenu()
{
    // Tertiary tries to make me cast... pshh, I'll just use good ol' if's. 
    if ( zm_cl_usenewmenus.GetBool() )
    {
        return m_pBuildMenuNew;
    }
    else
    {
        return m_pBuildMenu;
    }
}

CZMManiMenuBase* CZMViewOld::GetManiMenu()
{
    // Tertiary tries to make me cast... pshh, I'll just use good ol' if's. 
    if ( zm_cl_usenewmenus.GetBool() )
    {
        return m_pManiMenuNew;
    }
    else
    {
        return m_pManiMenu;
    }
}

void CZMViewOld::CloseChildMenus()
{
    if ( m_pBuildMenu && m_pBuildMenu->IsVisible() )
    {
        m_pBuildMenu->Close();
    }

    if ( m_pBuildMenuNew && m_pBuildMenuNew->IsVisible() )
    {
        m_pBuildMenuNew->Close();
    }

    if ( m_pManiMenu && m_pManiMenu->IsVisible() )
    {
        m_pManiMenu->Close();
    }

    if ( m_pManiMenuNew && m_pManiMenuNew->IsVisible() )
    {
        m_pManiMenuNew->Close();
    }
}
