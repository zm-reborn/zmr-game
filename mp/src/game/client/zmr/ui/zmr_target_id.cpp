//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"


#include "zmr/c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE            150
#define PLAYER_HINT_DISTANCE_SQ         (PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)


static ConVar zm_cl_showtargetid( "zm_cl_showtargetid", "1" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CZMTargetID : public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE( CZMTargetID, vgui::Panel );

public:
    CZMTargetID( const char *pElementName );
    void Init( void );
    virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
    virtual void	Paint( void );
    void VidInit( void );

private:
    Color			GetColorForTargetTeam( int iTeamNumber );

    vgui::HFont		m_hFont;
    int				m_iLastEntIndex;
    float			m_flLastChangeTime;
};

DECLARE_HUDELEMENT( CZMTargetID );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CZMTargetID::CZMTargetID( const char *pElementName ) :
    CHudElement( pElementName ), BaseClass( NULL, "TargetID" )
{
    vgui::Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );

    m_hFont = g_hFontTrebuchet24;
    m_flLastChangeTime = 0;
    m_iLastEntIndex = 0;

    SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CZMTargetID::Init( void )
{
};

void CZMTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
    BaseClass::ApplySchemeSettings( scheme );

    m_hFont = scheme->GetFont( "TargetID", IsProportional() );

    SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CZMTargetID::VidInit()
{
    CHudElement::VidInit();

    m_flLastChangeTime = 0;
    m_iLastEntIndex = 0;
}

Color CZMTargetID::GetColorForTargetTeam( int iTeamNumber )
{
    return COLOR_WHITE;
} 

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CZMTargetID::Paint()
{
    if ( !zm_cl_showtargetid.GetBool() ) return;


    C_ZMPlayer* pLocalPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pLocalPlayer ) return;

    if ( !pLocalPlayer->IsHuman() )
    {
        // Allow us to see people's target id even if we're dead.
        if (!pLocalPlayer->IsObserver()
        ||  pLocalPlayer->GetObserverTarget() == nullptr
        ||  pLocalPlayer->GetObserverMode() != OBS_MODE_IN_EYE )
        {
            return;
        }
    }


    // Get our target's ent index
    int iEntIndex = pLocalPlayer->GetIDTarget();
    // Didn't find one?
    if ( !iEntIndex )
    {
        // Check to see if we should clear our ID
        if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.5)) )
        {
            m_flLastChangeTime = 0;
            m_iLastEntIndex = 0;
        }
        else
        {
            // Keep re-using the old one
            iEntIndex = m_iLastEntIndex;
        }
    }
    else
    {
        m_flLastChangeTime = gpGlobals->curtime;
    }


    if ( !iEntIndex ) return;


    C_BasePlayer* pPlayer = ToBasePlayer( cl_entitylist->GetEnt( iEntIndex ) );

    if ( !pPlayer ) return;

    if ( !pPlayer->InSameTeam( pLocalPlayer ) ) return;


    wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
    wszPlayerName[0] = 0;

    wchar_t wszHealth[MAX_PLAYER_NAME_LENGTH];
    wszHealth[0] = 0;

#define MAX_ID_STRING 128
    wchar_t sOutput[ MAX_ID_STRING ];
    sOutput[0] = 0;

    const char* formHealth = nullptr;
    const char* formatString = "#ZMTargetId_Format";

    

    g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(),  wszPlayerName, sizeof( wszPlayerName ) );
    

    int hp = pPlayer->GetHealth();

    if ( hp > 80 )
    {
        formHealth = "#ZMTargetId_Healthy";
    }
    else if ( hp > 60 )
    {
        formHealth = "#ZMTargetId_Hurt";
    }
    else if ( hp > 30 )
    {
        formHealth = "#ZMTargetId_Wounded";
    }
    else if ( hp > 10 )
    {
        formHealth = "#ZMTargetId_NearDeath";
    }
    else
    {
        formHealth = "#ZMTargetId_Dead";
    }

    g_pVGuiLocalize->ConstructString( sOutput, sizeof( sOutput ), g_pVGuiLocalize->Find( formatString ), 2, wszPlayerName, g_pVGuiLocalize->Find( formHealth ) );

    if ( sOutput[0] )
    {
        // 640x480
        int ypos = YRES( 260 );
        int xpos = XRES( 10 );
            
        vgui::surface()->DrawSetTextFont( m_hFont );
        vgui::surface()->DrawSetTextPos( xpos, ypos );
        vgui::surface()->DrawSetTextColor( COLOR_WHITE );
        vgui::surface()->DrawPrintText( sOutput, wcslen( sOutput ) );
    }
}
