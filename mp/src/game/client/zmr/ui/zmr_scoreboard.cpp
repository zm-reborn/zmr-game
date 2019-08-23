//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_team.h"
#include "c_playerresource.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/SectionedListPanel.h>

#include "voice_status.h"
#include "vgui_avatarimage.h"
#include "inputsystem/iinputsystem.h"
#include "clientmode_shared.h"

#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/c_zmr_importancesystem.h"

#include "zmr_listpanel.h"
#include "zmr_scoreboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

// HACK
static char g_szHostname[256];


#define MAX_SCORE_HEIGHT            YRES( 450 )

static bool SortHumanSection( KeyValues* kv1, KeyValues* kv2 )
{
    if ( !kv1 || !kv2 ) return false;


    int p1 = kv1->GetInt( "playerIndex" );
    int p2 = kv2->GetInt( "playerIndex" );

    if ( p1 > 0 && p2 > 0 && g_PR )
    {
        // Spectators go last.
        if ( g_PR->GetTeam( p1 ) > g_PR->GetTeam( p2 ) )
            return true;
        else if ( g_PR->GetTeam( p1 ) < g_PR->GetTeam( p2 ) )
            return false;
    }

    // We have more score?
    int f1 = kv1->GetInt( "frags" );
    int f2 = kv2->GetInt( "frags" );
    if ( f1 > f2 )
        return true;
    else if ( f1 < f2 )
        return false;

    // We have less deaths?
    if ( kv1->GetInt( "deaths" ) < kv2->GetInt( "deaths" ) )
        return true;

    return false;
}

static bool SortZMSection( KeyValues* kv1, KeyValues* kv2 )
{
    if ( !kv1 || !kv2 ) return false;


    return ( kv1->GetInt( "zmkills" ) > kv2->GetInt( "zmkills" ) );
}


int CZMAvatarList::CreateAvatarBySteamId( CSteamID id )
{
    int index = -1;

    if ( (index = FindAvatarBySteamId( id )) != -1 )
        return index;


    CAvatarImage* pImage = new CAvatarImage();
    pImage->SetDrawFriend( false );
    pImage->SetSize( 32, 32 );
    pImage->SetAvatarSteamID( id );

    return m_ImageList.AddToTail( zm_avatarimg_t( id, pImage ) );
}

int CZMAvatarList::FindAvatarBySteamId( CSteamID id )
{
    int len = m_ImageList.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_ImageList[i].SteamId == id )
            return i;
    }

    return -1;
}


// NOTE: We have to be a frame to enable mouse input.
CZMClientScoreBoardDialog::CZMClientScoreBoardDialog( IViewPort* pViewPort ) : CZMFramePanel( nullptr, PANEL_SCOREBOARD )
{
    // Has to be set to load fonts correctly.
    SetScheme( vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" ) );


    m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString( "playerIndex" );


    ListenForGameEvent( "server_spawn" );


    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( false );
    SetProportional( true );


    m_flNextUpdateTime = 0.0f;

    m_nTexBgSideId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgSideId, "zmr_effects/hud_bg_score_side", true, false );

    m_nTexBgTopId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgTopId, "zmr_effects/hud_bg_score_top", true, false );

    m_nTexBgCornerId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgCornerId, "zmr_effects/hud_bg_score_corner", true, false );


    LoadControlSettings( "resource/ui/zmscoreboard.res" );


    m_pList = dynamic_cast<CZMListPanel*>( FindChildByName( "PlayerList" ) );
    Assert( m_pList );

    m_iSectionZM = m_pList->GetSectionByName( "SectionTeamZM" );
    m_iSectionHuman = m_pList->GetSectionByName( "SectionTeamHuman" );
    Assert( m_iSectionHuman != -1 && m_iSectionZM != -1 );

    m_pList->SetSectionSortingFunc( m_iSectionZM, SortZMSection );
    m_pList->SetSectionSortingFunc( m_iSectionHuman, SortHumanSection );

    m_pList->SetMouseInputEnabled( true );
    m_pList->SetSectionMouseInputEnabled( m_iSectionHuman, true );
    m_pList->SetSectionMouseInputEnabled( m_iSectionZM, true );


    m_iVoiceOff = m_pList->AddImage( scheme()->GetImage( "zmr_misc/voice_off", true ) );
    m_iVoiceOn = m_pList->AddImage( scheme()->GetImage( "zmr_misc/voice_on", true ) );


    g_ZMImportanceSystem.InitImages();
}

CZMClientScoreBoardDialog::~CZMClientScoreBoardDialog()
{
}

void CZMClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );


    SetBgColor( vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "ZMScoreboardBg", COLOR_BLACK ) );
}

void CZMClientScoreBoardDialog::FireGameEvent( IGameEvent* event )
{
    if ( Q_strcmp( event->GetName(), "server_spawn" ) == 0 )
    {
        m_pList->ClearRows( m_iSectionHuman );
        m_pList->ClearRows( m_iSectionZM );

        // We'll post the message ourselves instead of using SetControlString()
        // so we don't try to translate the hostname.
        const char* hostname = event->GetString( "hostname" );

        Q_strncpy( g_szHostname, hostname, sizeof( g_szHostname ) );

        Panel* control = FindChildByName( "ServerName" );
        if ( control )
        {
            PostMessage( control, new KeyValues( "SetText", "text", hostname ) );
            control->MoveToFront();
        }
    }
}

void CZMClientScoreBoardDialog::OnListLayout( KeyValues* kv )
{
    PerformLayout();
}

void CZMClientScoreBoardDialog::OnRowItemPressed( KeyValues* kv )
{
#ifdef _DEBUG
    DevMsg( "On Row Item Pressed (Scoreboard) %s\n", kv->GetString( "pressed_name" ) );
#endif
    if ( Q_stricmp( "mutestatus", kv->GetString( "pressed_name" ) ) == 0 )
    {
        ToggleVoiceMute( kv->GetInt( m_iPlayerIndexSymbol ) );
    }
}

void CZMClientScoreBoardDialog::OnThink()
{
    BaseClass::OnThink();
    

    // HACK: Enable mouse input on right click.
    if ( IsVisible() && g_pInputSystem->IsButtonDown( MOUSE_RIGHT ) && (gpGlobals->curtime - m_flLastMouseToggle) > 0.2f )
    {
        bool toggle = !IsMouseInputEnabled();

        SetMouseInputEnabled( toggle );

        m_flLastMouseToggle = gpGlobals->curtime;
    }
}

void CZMClientScoreBoardDialog::Reset()
{
    Update();
    UpdateStats();

    PerformLayout();
}

void CZMClientScoreBoardDialog::Update()
{
    UpdateScoreboard();

    m_flNextUpdateTime = gpGlobals->curtime + 0.5f;
}

bool CZMClientScoreBoardDialog::NeedsUpdate()
{
    return m_flNextUpdateTime < gpGlobals->curtime;
}

void CZMClientScoreBoardDialog::ShowPanel( bool bShow )
{
    if ( IsVisible() == bShow )
        return;


    if ( bShow )
    {
        Reset();
        Update();
        SetVisible( true );
        MoveToFront();
    }
    else
    {
        SetVisible( false );
        SetMouseInputEnabled( false );

        m_flLastMouseToggle = 0.0f;
    }
}

void CZMClientScoreBoardDialog::PerformLayout()
{
    // Set size
    int max_h = MAX_SCORE_HEIGHT;
    int min_h = YRES( 270 );

    int x, y, w, h;
    m_pList->GetBounds( x, y, w, h );


    y += h + YRES( 32 );
    if ( y < min_h )
        y = min_h;
    else if ( y > max_h )
        y = max_h;


    SetSize( GetWide(), y );

    // Center
    SetPos( (ScreenWidth() - GetWide()) / 2, (ScreenHeight() - MAX_SCORE_HEIGHT) / 2 );
}

void CZMClientScoreBoardDialog::PaintBackground()
{
    // Alright, time for another story
    // For the longest time, we've had problems with translucent UI materials having sorting issues.
    // I had never seen it this bad and seemingly for no reason.
    // I've tried every fucking combination of material parameters ($alphatest,$distancetoalpha...)
    // and trying to debug the panel class for hours.
    // Only recently I noticed that this problem is only encountered with frame panels...
    // Well, after taking a look in the LEAKED engine vgui code, I was able to fix it with this shit.
    // Tbh, I should've been able to fix this earlier...
    //
    // Popup panels are drawn separately with their own stencil reference value
    // to make them render on top of other panels.
    // WE don't want this, but we need the popup ability to toggle mouse whenever we want.
    // Hence, just set the value to 0
    //
    // Thanks Valve! (this is my 7th "thanks valve", and I'm sure there's more to come)
    CMatRenderContextPtr pRenderContext( materials );
    pRenderContext->SetStencilReferenceValue( 0 );


    // Paint fancy border.

    int x, y;
    int w, h;
    GetSize( w, h );

    int repeats;
    float tx1, ty1;


    const Color clr = GetBgColor();

    // Actual length of texture.
    const int nBorderTexLength = 512;

    const int border_w = XRES( 32 );
    const int border_h = YRES( 32 );


    x = border_w;
    y = border_h;
    w -= border_w * 2;
    h -= border_h * 2;


    repeats = (int)(nBorderTexLength / (float)h);
    if ( repeats < 1 ) repeats = 1;

    ty1 = (float)repeats;


    surface()->DrawSetColor( clr );

    // Left
    surface()->DrawSetTexture( m_nTexBgSideId );
    surface()->DrawTexturedSubRect( 0, border_h, border_w, border_h + h, 0.0f, 0.0f, 1.0f, ty1 );

    // Right
    surface()->DrawSetTexture( m_nTexBgSideId );
    surface()->DrawTexturedSubRect( border_w + w, border_h, border_w + w + border_w, border_h + h, 1.0f, 1.0f, 0.0f, (-ty1) + 1.0f );
    

    repeats = (int)(nBorderTexLength / (float)w);
    if ( repeats < 1 ) repeats = 1;

    tx1 = (float)repeats;

    // Top
    surface()->DrawSetTexture( m_nTexBgTopId );
    surface()->DrawTexturedSubRect( border_w, 0, border_w + w, border_h, 0.0f, 0.0f, tx1, 1.0f );

    // Bottom
    surface()->DrawSetTexture( m_nTexBgTopId );
    surface()->DrawTexturedSubRect( border_w, border_h + h, border_w + w, border_h + h + border_h, 1.0f, 1.0f, (-tx1) + 1.0f, 0.0f );


    // Top left corner
    surface()->DrawSetTexture( m_nTexBgCornerId );
    surface()->DrawTexturedRect( 0, 0, border_w, border_h );

    // Top right corner
    surface()->DrawSetTexture( m_nTexBgCornerId );
    surface()->DrawTexturedSubRect( border_w + w, 0, border_w + w + border_w, border_h, 1.0f, 0.0f, 0.0f, 1.0f );

    // Bottom left corner
    surface()->DrawSetTexture( m_nTexBgCornerId );
    surface()->DrawTexturedSubRect( 0, border_h + h, border_w, border_h + h + border_h, 0.0f, 1.0f, 1.0f, 0.0f );

    // Bottom right corner
    surface()->DrawSetTexture( m_nTexBgCornerId );
    surface()->DrawTexturedSubRect( border_w + w, border_h + h, border_w + w + border_w, border_h + h + border_h, 1.0f, 1.0f, 0.0f, 0.0f );


    // Actual background.
    surface()->DrawFilledRect( x, y, x + w, y + h );
}

void CZMClientScoreBoardDialog::ToggleVoiceMute( int playerIndex )
{
    GetClientVoiceMgr()->SetPlayerBlockedState( playerIndex, !GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) );
}

int CZMClientScoreBoardDialog::FindPlayerItem( int playerIndex )
{
    return m_pList->FindItemByKey( m_iPlayerIndexSymbol, playerIndex );
}

void CZMClientScoreBoardDialog::UpdateStats()
{
    Label* pInfo = dynamic_cast<Label*>( FindChildByName( "MinorInfo" ) );
    if ( pInfo )
    {
        char tempstr[128];
        wchar_t buf[256];
        wchar_t wMapName[64];


        Q_FileBase( engine->GetLevelName(), tempstr, sizeof( tempstr ) );

        g_pVGuiLocalize->ConvertANSIToUnicode( tempstr, wMapName, sizeof( wMapName ) );
        g_pVGuiLocalize->ConstructString( buf, sizeof( buf ), g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

        pInfo->SetText( buf );
    }

    Label* pHostnameLabel = dynamic_cast<Label*>( FindChildByName( "ServerName" ) );
    if ( pHostnameLabel )
    {
        pHostnameLabel->SetText( g_szHostname );
    }
}

void CZMClientScoreBoardDialog::UpdateMapStats()
{
    Label* pRoundLabel = dynamic_cast<Label*>( FindChildByName( "RoundLabel" ) );
    if ( pRoundLabel )
    {
        wchar_t buf[64];
        wchar_t round[16];

        V_snwprintf( round, sizeof( round ), L"%i", ZMRules()->GetRoundsPlayed() + 1 );

        g_pVGuiLocalize->ConstructString( buf, sizeof( buf ), g_pVGuiLocalize->Find( "#ZMRoundCount" ), 1, round );

        pRoundLabel->SetText( buf );
    }
}

void CZMClientScoreBoardDialog::UpdateScoreboard()
{
    UpdatePlayerInfo();
    UpdateMapStats();
}

int CZMClientScoreBoardDialog::TeamToSection( int iTeam )
{
    switch ( iTeam )
    {
    case ZMTEAM_ZM : return m_iSectionZM;
    case ZMTEAM_HUMAN :
    default :
        return m_iSectionHuman;
    }
}

void CZMClientScoreBoardDialog::UpdatePlayerInfo()
{
    if ( !g_PR ) return;


    for ( int i = 1; i <= gpGlobals->maxClients; ++i )
    {
        if ( !g_PR->IsConnected( i ) )
        {
            // Remove the player.
            int itemId = FindPlayerItem( i );

            if ( itemId != -1 )
            {
                m_pList->RemoveItemById( itemId );
            }

            continue;
        }


        int iTeam = g_PR->GetTeam( i );

        // Get the player data.
        KeyValues* data = new KeyValues( "data" );
        GetPlayerScoreInfo( i, data );
        UpdatePlayerAvatar( i, data );

        const char* oldName = data->GetString( "name", "" );
        char newName[MAX_PLAYER_NAME_LENGTH * 2 + 1];

        UTIL_MakeSafeName( oldName, newName, sizeof( newName ) );

        data->SetString( "name", newName );


        int itemId = FindPlayerItem( i );
            
        //if ( g_PR->IsLocalPlayer( i ) )
        //{
        //    selectedRow = itemId;
        //}

        int section = TeamToSection( iTeam );
        int itemIndex = -1;

        if ( itemId == -1 )
        {
            // Add new row.
            itemIndex = m_pList->AddItem( section, data );
        }
        else
        {
            // Modify the current row.
            itemIndex = m_pList->ModifyItem( itemId, section, data );
        }

        if ( itemIndex != -1 )
        {
            m_pList->SetItemColor( section, itemIndex, g_PR->GetTeamColor( iTeam ) );
        }
    }
}

void CZMClientScoreBoardDialog::GetPlayerScoreInfo( int playerIndex, KeyValues* kv )
{
    kv->SetInt( "playerIndex", playerIndex ); // Must always be set.

    kv->SetString( "name", g_PR->GetPlayerName( playerIndex ) );

    // ZMRTODO: Add ZM kills.
    kv->SetInt( "zmkills", g_PR->GetPlayerScore( playerIndex ) );

    kv->SetInt( "frags", g_PR->GetPlayerScore( playerIndex ) );

	kv->SetInt( "deaths", g_PR->GetDeaths( playerIndex ) );


    bool bIsLocal = GetLocalPlayerIndex() == playerIndex;
    bool bIsBot = g_PR->IsFakePlayer( playerIndex );

    if ( bIsBot )
        kv->SetString( "ping", "BOT" );
    else
        kv->SetInt( "ping", g_PR->GetPing( playerIndex ) );


    int iMuteIcon = -1;
#ifdef _DEBUG // For testing
    if ( bIsLocal == bIsLocal )
#else
    if ( !bIsLocal && !bIsBot )
#endif
        iMuteIcon = GetClientVoiceMgr()->IsPlayerBlocked( playerIndex ) ? m_iVoiceOff : m_iVoiceOn;


    kv->SetInt( "mutestatus", iMuteIcon );


    // Importance
    auto* pImage = g_ZMImportanceSystem.GetPlayerImportanceImageIndex( playerIndex );
    int iImage = -1;

    if ( pImage )
    {
        iImage = m_pList->AddImage( pImage );
    }

    kv->SetInt( "importance", iImage );

    kv->SetString( "importance_tooltip", g_ZMImportanceSystem.GetPlayerImportanceName( playerIndex ) );
}

void CZMClientScoreBoardDialog::UpdatePlayerAvatar( int playerIndex, KeyValues* kv )
{
    // Update their avatar
    if ( kv /*&& ShowAvatars()*/ && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
    {
        player_info_t pi;
        if ( engine->GetPlayerInfo( playerIndex, &pi ) )
        {
            if ( pi.friendsID )
            {
                CSteamID SteamId( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );


                int index = m_Avatars.CreateAvatarBySteamId( SteamId );

                kv->SetInt( "avatar", m_pList->AddImage( m_Avatars.GetImage( index ) ) );
            }
        }
    }
}
