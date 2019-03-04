#include "cbase.h"

#include <vgui/ISurface.h>

#include "zmr_mainmenu.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu_subbtn.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


DECLARE_BUILD_FACTORY( CZMMainMenuButton );


extern CUtlSymbolTable g_ButtonSoundNames;

using namespace vgui;


CZMMainMenuButton::CZMMainMenuButton( Panel* pParent, const char* name ) : BaseClass( pParent, name )
{
    m_bOnlyInGame = false;

    m_nMaxSubTextWidth = 0;
    m_nSubBtnHeight = 0;
    m_flArmedTime = 0.0f;
    m_flUnarmedTime = 0.0f;
}

CZMMainMenuButton::~CZMMainMenuButton()
{
    m_vSubBtns.PurgeAndDeleteElements();
}

void CZMMainMenuButton::DoClick()
{
    if ( !IsArmed() || !m_vSubBtns.Count() )
    {
        BaseClass::DoClick();
    }
}

void CZMMainMenuButton::OnCursorExited()
{
    if ( !m_vSubBtns.Count() )
    {
        BaseClass::OnCursorExited();
    }
}

void CZMMainMenuButton::SetSelected( bool state )
{
    if ( _buttonFlags.IsFlagSet( SELECTED ) != state )
    {
        _buttonFlags.SetFlag( SELECTED, state );
        RecalculateDepressedState();
        InvalidateLayout( false );
    }

    SetArmed( state );
}

void CZMMainMenuButton::SetArmed( bool state )
{
    if ( _buttonFlags.IsFlagSet( ARMED ) != state )
    {
        _buttonFlags.SetFlag( ARMED, state );

        // Play any sounds specified
        if ( state && m_sArmedSoundName != UTL_INVAL_SYMBOL )
        {
            surface()->PlaySound( g_ButtonSoundNames.String( m_sArmedSoundName ) );
        }


        // Hide all other sub buttons
        if ( state )
        {
            GetMainMenu()->HideSubButtons( this );
        }


        if ( state )
        {
            m_flArmedTime = gpGlobals->realtime;
            m_flUnarmedTime = 0.0f;
            ShowSubButtons();
        }
        else
        {
            m_flArmedTime = 0.0f;
            m_flUnarmedTime = gpGlobals->realtime;
            HideSubButtons();
        }

        RecalculateDepressedState();
        InvalidateLayout( false );
    }
}

void CZMMainMenuButton::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    //PositionSubButtons();
}

void CZMMainMenuButton::ApplySettings( KeyValues* kv )
{
    m_bOnlyInGame = kv->GetBool( "onlyingame" );
    m_bOnlyNotInGame = kv->GetBool( "onlynotingame" );


    KeyValues* subkv = kv->FindKey( "subbuttons" );
    if ( subkv )
    {
        for ( subkv = subkv->GetFirstTrueSubKey(); subkv; subkv = subkv->GetNextTrueSubKey() )
        {
            AddSubButton( subkv );
        }
    }

    BaseClass::ApplySettings( kv );
}

void CZMMainMenuButton::PerformLayout()
{
    BaseClass::PerformLayout();


    ComputeMaxTextWidth();
}

void CZMMainMenuButton::ComputeMaxTextWidth()
{
    int w, h;

    m_nMaxSubTextWidth = 0;

    int len = m_vSubBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_vSubBtns[i]->GetContentSize( w, h );
        if ( m_nMaxSubTextWidth < w )
        {
            m_nMaxSubTextWidth = w;
        }
    }

    if ( !m_nMaxSubTextWidth )
    {
        GetContentSize( m_nMaxSubTextWidth, h );

        if ( !m_nMaxSubTextWidth )
            m_nMaxSubTextWidth = GetWide();
    }

    Assert( m_nMaxSubTextWidth > 0 );
}

bool CZMMainMenuButton::IsSubButtonsVisible() const
{
    FOR_EACH_VEC( m_vSubBtns, i )
    {
        if ( m_vSubBtns[i]->IsVisible() && !m_vSubBtns[i]->IsFadingOut() )
            return true;
    }

    return false;
}

void CZMMainMenuButton::AttemptToShowButtons()
{
    if ( !m_vSubBtns.Count() )
        return;


    GetMainMenu()->HideSubButtons( this );

    if ( !IsSubButtonsVisible() )
        ShowSubButtons();
}

void CZMMainMenuButton::ShowSubButtons()
{
    PositionSubButtons();


    FOR_EACH_VEC( m_vSubBtns, i )
    {
        m_vSubBtns[i]->FadeIn( 0.1f + (i * 0.1f) );
    }

    GetParent()->Repaint();
}

void CZMMainMenuButton::HideSubButtons()
{
    int len = m_vSubBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_vSubBtns[i]->FadeOut( 0.1f + ((len-i-1) * 0.1f) );
    }

    GetParent()->Repaint();
}

void CZMMainMenuButton::PositionSubButtons()
{
    int px, py;
    GetPos( px, py );

    const int size_x = GetWide();
    const int size_y = GetTall() * 0.6f;


    m_nSubBtnHeight = size_y;


    const int offset_x = 0;
    const int offset_y = -size_y;

    int len = m_vSubBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_vSubBtns[i]->SetPos(
            px + offset_x,
            py + offset_y - i * size_y );
        m_vSubBtns[i]->SetSize( size_x, size_y );
    }
}

void CZMMainMenuButton::AddSubButton( const char* label, const char* command )
{
    auto* pBtn = new CZMMainMenuSubButton( this, "" );
    pBtn->SetText( label );
    pBtn->SetVisible( false );
    pBtn->SetCommand( command );

    m_vSubBtns.AddToTail( pBtn );
}

void CZMMainMenuButton::AddSubButton( KeyValues* kv )
{
    auto* pBtn = new CZMMainMenuSubButton( this, "" );

    pBtn->ApplySettings( kv );

    pBtn->SetVisible( false );

    m_vSubBtns.AddToTail( pBtn );
}
