#include "cbase.h"
#include "backgroundpanel.h"
#include "IGameUIFuncs.h"


#include "zmr_textwindow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern IGameUIFuncs* gameuifuncs;

using namespace vgui;


CZMTextWindow::CZMTextWindow( IViewPort* pViewPort ) : CTextWindow( pViewPort )
{
    SetProportional( true );

    m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

    CreateBackground( this );
    m_backgroundLayoutFinished = false;
}

CZMTextWindow::~CZMTextWindow()
{
}

void CZMTextWindow::Update()
{
    BaseClass::Update();

    m_pOK->RequestFocus();
}

void CZMTextWindow::SetVisible( bool state )
{
    BaseClass::SetVisible(state);

    if ( state )
    {
        m_pOK->RequestFocus();
    }
}

void CZMTextWindow::ShowPanel( bool bShow )
{
    if ( bShow )
    {
        // get key binding if shown
        if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) // you need to lookup the jump key AFTER the engine has loaded
        {
            m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
        }
    }

    BaseClass::ShowPanel( bShow );
}

void CZMTextWindow::OnKeyCodePressed( KeyCode code )
{
    if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
    {
        gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
        gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
    }
    else
    {
        BaseClass::OnKeyCodePressed( code );
    }
}

void CZMTextWindow::PaintBackground()
{
}

void CZMTextWindow::PerformLayout()
{
    BaseClass::PerformLayout();

    // stretch the window to fullscreen
    if ( !m_backgroundLayoutFinished )
        LayoutBackgroundPanel( this );
    m_backgroundLayoutFinished = true;
}

void CZMTextWindow::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );
    ApplyBackgroundSchemeSettings( this, pScheme );
}
