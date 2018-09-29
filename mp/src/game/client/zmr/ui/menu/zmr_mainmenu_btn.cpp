#include "cbase.h"

#include "zmr_mainmenu.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu_subbtn.h"




DECLARE_BUILD_FACTORY( CZMMainMenuButton );


using namespace vgui;


CZMMainMenuButton::CZMMainMenuButton( Panel* pParent, const char* name ) : BaseClass( pParent, name )
{
    m_bOnlyInGame = false;
}

CZMMainMenuButton::~CZMMainMenuButton()
{
    m_vSubBtns.RemoveAll();
}

void CZMMainMenuButton::OnCursorEntered()
{
    AttemptToShowButtons();

    BaseClass::OnCursorEntered();
}

void CZMMainMenuButton::OnCursorExited()
{
    //HideSubButtons();

    BaseClass::OnCursorExited();
}

void CZMMainMenuButton::OnMousePressed( MouseCode code )
{
    if ( m_vSubBtns.Count() )
    {
        GetMainMenu()->HideSubButtons( this );

        if ( !IsSubButtonsVisible() )
            ShowSubButtons();
        else
            HideSubButtons();

        return;
    }

    BaseClass::OnMousePressed( code );
}

void CZMMainMenuButton::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );


    //PositionSubButtons();
}

void CZMMainMenuButton::ApplySettings( KeyValues* kv )
{
    m_bOnlyInGame = kv->GetBool( "onlyingame" );


    KeyValues* subkv = kv->FindKey( "subbuttons" );
    if ( subkv )
    {
        subkv = subkv->GetFirstSubKey();

        while ( subkv )
        {
            AddSubButton( subkv->GetName(), subkv->GetString() );

            subkv = subkv->GetNextKey();
        }
    }

    BaseClass::ApplySettings( kv );
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
}

void CZMMainMenuButton::HideSubButtons()
{
    int len = m_vSubBtns.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_vSubBtns[i]->FadeOut( 0.1f + ((len-i-1) * 0.1f) );
    }
}

void CZMMainMenuButton::PositionSubButtons()
{
    int px, py;
    GetPos( px, py );

    const int size_x = GetWide();
    const int size_y = GetTall() * 0.6f;

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
