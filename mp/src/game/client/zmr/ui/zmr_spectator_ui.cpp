#include "cbase.h"
#include <c_playerresource.h>

#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>


#include "zmr_spectator_ui.h"


using namespace vgui;


CZMSpectatorGUI::CZMSpectatorGUI( IViewPort* pViewPort ) : CSpectatorGUI( pViewPort )
{
}

bool CZMSpectatorGUI::NeedsUpdate( void )
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer )
        return false;


    if ( m_nLastSpecMode != pPlayer->GetObserverMode() )
        return true;

    C_BaseEntity* pTarget = pPlayer->GetObserverTarget();

    if ( m_pLastSpecTarget != pTarget )
        return true;

    if ( pTarget )
    {
        if ( pTarget->GetHealth() != m_nLastSpecHealth )
            return true;
    }

    return BaseClass::NeedsUpdate();
}

void CZMSpectatorGUI::Update()
{
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();

    if ( !pLocal ) return;



    C_BaseEntity* pTarget = pLocal->GetObserverTarget();


    int wide, tall;
    int bx, by, bwide, btall;

    GetHudSize( wide, tall );
    m_pTopBar->GetBounds( bx, by, bwide, btall );


    int specmode = GetSpectatorMode();
    int playernum = pTarget ? pTarget->entindex() : 0;


    IViewPortPanel* overview = gViewPortInterface->FindPanelByName( PANEL_OVERVIEW );

    if ( overview && overview->IsVisible() )
    {
        int mx, my, mwide, mtall;

        VPANEL p = overview->GetVPanel();
        vgui::ipanel()->GetPos( p, mx, my );
        vgui::ipanel()->GetSize( p, mwide, mtall );
                
        if ( my < btall )
        {
            // reduce to bar 
            m_pTopBar->SetSize( wide - (mx + mwide), btall );
            m_pTopBar->SetPos( (mx + mwide), 0 );
        }
        else
        {
            // full top bar
            m_pTopBar->SetSize( wide , btall );
            m_pTopBar->SetPos( 0, 0 );
        }
    }
    else
    {
        // full top bar
        m_pTopBar->SetSize( wide , btall ); // change width, keep height
        m_pTopBar->SetPos( 0, 0 );
    }

    m_pPlayerLabel->SetVisible( ShouldShowPlayerLabel( specmode ) );

    // update player name filed, text & color

    if ( playernum > 0 && playernum <= gpGlobals->maxClients && g_PR && pTarget )
    {
        Color c = g_PR->GetTeamColor( g_PR->GetTeam( playernum ) ); // Player's team color

        m_pPlayerLabel->SetFgColor( c );
        
        wchar_t playerText[ 80 ], playerName[ 64 ], health[ 10 ];
        V_wcsncpy( playerText, L"Unable to find #Spec_PlayerItem*", sizeof( playerText ) );
        memset( playerName, 0x0, sizeof( playerName ) );

        g_pVGuiLocalize->ConvertANSIToUnicode( UTIL_SafeName( g_PR->GetPlayerName( playernum ) ), playerName, sizeof( playerName ) );


        // We have to use the hp directly since the health updated by player resource is slow.
        int iHealth = pTarget->GetHealth();

        if ( iHealth > 0 && pTarget->IsAlive() )
        {
            _snwprintf( health, ARRAYSIZE( health ), L"%i", iHealth );
            g_pVGuiLocalize->ConstructString( playerText, sizeof( playerText ), g_pVGuiLocalize->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  health );
        }
        else
        {
            g_pVGuiLocalize->ConstructString( playerText, sizeof( playerText ), g_pVGuiLocalize->Find( "#Spec_PlayerItem" ), 1, playerName );
        }

        m_pPlayerLabel->SetText( playerText );
    }
    else
    {
        m_pPlayerLabel->SetText( L"" );
    }



    // ZMRTODO: Put more info here.
    // Update map name & move it so it doesn't get clipped.
    Label* pLabel = dynamic_cast<Label*>( FindChildByName( "extrainfo" ) );

    if ( pLabel )
    {
        char tempstr[128];
        wchar_t buf[256];
        wchar_t wMapName[64];


        Q_FileBase( engine->GetLevelName(), tempstr, sizeof(tempstr) );

        g_pVGuiLocalize->ConvertANSIToUnicode( tempstr,wMapName, sizeof( wMapName ) );
        g_pVGuiLocalize->ConstructString( buf, sizeof( buf ), g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

        pLabel->SetText( buf );


        // Move it.
        int w, h;
        pLabel->GetTextImage()->GetContentSize( w, h );

        pLabel->SetPos( ScreenWidth() - w - XRES( 2 ), GetYPos() );
    }


    // Update our cache.
    m_nLastSpecHealth = pTarget ? pTarget->GetHealth() : 0;
    m_nLastSpecMode = specmode;
    m_pLastSpecTarget = pTarget;
}
