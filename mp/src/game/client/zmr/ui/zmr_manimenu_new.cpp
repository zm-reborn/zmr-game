#include "cbase.h"
#include <vgui/ILocalize.h>

#include "zmr_radial.h"
#include "zmr_manimenu_new.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


CZMManiMenuNew::CZMManiMenuNew( Panel* pParent ) : CZMManiMenuBase( "ZMManiMenu" )
{
    SetParent( pParent->GetVPanel() );


    SetTrapIndex( 0 );


    SetPaintBackgroundEnabled( false );
    SetSizeable( false );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );
    DisableMouseInputForThisPanel( true ); // Only THIS panel can't be clicked. Children work fine.
    SetProportional( true );
    SetMoveable( false );


	//SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ZombieMaster.res", "ZombieMaster" ) );

	LoadControlSettings( "resource/ui/zmmanimenunew.res" );

    m_pRadial = dynamic_cast<CZMRadialPanel*>( FindChildByName( "ZMRadialPanel1" ) );
    Assert( m_pRadial );
    m_pRadial->LoadFromFile( "resource/zmradial_trap.txt" );
    m_pRadial->SetBackgroundImage( "zmr_manimenu/bg" );


    m_pDescLabel = dynamic_cast<Label*>( FindChildByName( "Description" ) );
    Assert( m_pDescLabel );
    m_pDescLabel->SetMouseInputEnabled( false );
}

CZMManiMenuNew::~CZMManiMenuNew()
{

}

void CZMManiMenuNew::Paint()
{
    // HACK:
    // See scoreboard Paint() for rant
    CMatRenderContextPtr pRenderContext( materials );
    pRenderContext->SetStencilReferenceValue( 0 );


    BaseClass::Paint();
}

void CZMManiMenuNew::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;

    if ( state )
    {
        // Notify the server we've opened this menu.
        engine->ClientCmd( VarArgs( "zm_cmd_openmenu %i", GetTrapIndex() ) );
    }

    SetVisible( state );
}

void CZMManiMenuNew::SetDescription( const char* desc )
{
    m_pDescLabel->SetText( desc );
}

void CZMManiMenuNew::SetCost( int cost )
{
    wchar_t buffer[128];
    wchar_t wcost[32];

    V_snwprintf( wcost, sizeof( wcost ), L"%i", cost );
    g_pVGuiLocalize->ConstructString( buffer, sizeof( buffer ), g_pVGuiLocalize->Find( "#ZMManiActivate" ), 1, wcost );

    Label* entry = dynamic_cast<Label*>( FindChildByName( "Activate", true ) );
    
    if ( entry )
    {
        entry->SetText( buffer );
    }

    m_nCost = cost;
}

void CZMManiMenuNew::SetTrapCost( int cost )
{
    wchar_t buffer[128];

    if ( cost > 0 )
    {
        wchar_t wcost[32];

        V_snwprintf( wcost, sizeof( wcost ), L"%i", cost );
        g_pVGuiLocalize->ConstructString( buffer, sizeof( buffer ), g_pVGuiLocalize->Find( "#ZMManiTrap" ), 1, wcost );
    }
    else
    {
        // Are traps gay?
        g_pVGuiLocalize->ConstructString( buffer, sizeof( buffer ), g_pVGuiLocalize->Find( "#ZMManiTrapDisabled" ), 0 );
    }
    

    Label* entry = dynamic_cast<Label*>( FindChildByName( "Trap", true ) );
    
    if ( entry )
    {
        entry->SetText( buffer );
    }

    m_nTrapCost = cost;
}

void CZMManiMenuNew::ShowMenu( C_ZMEntManipulate* pMani )
{
    SetWorldPos( pMani->GetAbsOrigin() );


    

    SetOffset(
        m_pRadial->GetXPos() + m_pRadial->GetWide() / 2.0f,
        m_pRadial->GetYPos() + m_pRadial->GetTall() / 2.0f );
    SetLimits(
        m_pRadial->GetXPos(),
        0,
        m_pRadial->GetXPos() + m_pRadial->GetWide(),
        m_pRadial->GetYPos() + m_pRadial->GetTall() );

    BaseClass::ShowMenu( pMani );
}

void CZMManiMenuNew::OnThink()
{
	if ( !IsVisible() ) return;


    MoveToFront();


	C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
	{
        return;
	}

    CUtlVector<CZMRadialButton*>* pButtons = m_pRadial->GetButtons();
    Assert( pButtons->Count() >= 2 );

    pButtons->Element( 0 )->SetDisabled( pPlayer->GetResources() < m_nCost );
    pButtons->Element( 1 )->SetDisabled( m_nTrapCost <= 0 || pPlayer->GetResources() < m_nTrapCost );


    int x, y;
    GetPos( x, y );
    GetScreenPos( x, y );

    SetPos( x, y );
}
