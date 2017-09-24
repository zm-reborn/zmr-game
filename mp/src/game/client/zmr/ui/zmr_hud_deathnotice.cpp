#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_baseplayer.h"
#include "c_team.h"


#include "zmr/zmr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;



static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );



// Player entries in a death notice
struct DeathNoticePlayer
{
    char        szName[MAX_PLAYER_NAME_LENGTH];
    //int         iEntIndex;
    int         iTeam;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
    DeathNoticePlayer	Killer;
    DeathNoticePlayer   Victim;
    CHudTexture*        iconDeath;
    bool                bAloneDeath;
    float               flDisplayTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CZMHudDeathNotice : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE( CZMHudDeathNotice, Panel );
public:
    CZMHudDeathNotice( const char* pElementName );

    void Init();
    void VidInit();
    virtual bool ShouldDraw();
    virtual void Paint();
    virtual void ApplySchemeSettings( IScheme* scheme );

    void SetColorForNoticePlayer( int iTeamNumber );
    void RetireExpiredDeathNotices();
    
    virtual void FireGameEvent( IGameEvent* pEvent );

private:

    CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "15", "proportional_float" );

    CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );

    CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );

    CPanelAnimationVar( HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );


    // Texture for skull symbol
    CHudTexture*    m_iconD_skull;  
    CHudTexture*    m_iconD_headshot;  

    CUtlVector<DeathNoticeItem> m_DeathNotices;
};

DECLARE_HUDELEMENT( CZMHudDeathNotice );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CZMHudDeathNotice::CZMHudDeathNotice( const char* pElementName ) :
    CHudElement( pElementName ), BaseClass( nullptr, "HudDeathNotice" )
{
    SetParent( g_pClientMode->GetViewport() );

    m_iconD_headshot = NULL;
    m_iconD_skull = NULL;

    SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
    BaseClass::ApplySchemeSettings( scheme );
    SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::Init()
{
    ListenForGameEvent( "player_death" );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::VidInit()
{
    m_iconD_skull = gHUD.GetIcon( "d_skull" );
    m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CZMHudDeathNotice::ShouldDraw()
{
    return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::SetColorForNoticePlayer( int iTeamNumber )
{
    surface()->DrawSetTextColor( g_PR->GetTeamColor( iTeamNumber ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::Paint()
{
    if ( !m_iconD_skull )
        return;


    CBaseViewport* pViewport = dynamic_cast<CBaseViewport*>( g_pClientMode->GetViewport() );

    int yStart = YRES( 2 );

    if ( pViewport )
        yStart = pViewport->GetDeathMessageStartHeight();


    surface()->DrawSetTextFont( m_hTextFont );
    surface()->DrawSetTextColor( g_PR->GetTeamColor( 0 ) );


    int iCount = m_DeathNotices.Count();
    for ( int i = 0; i < iCount; i++ )
    {
        CHudTexture *icon = m_DeathNotices[i].iconDeath;
        if ( !icon )
            continue;

        wchar_t victim[ 256 ];
        wchar_t killer[ 256 ];

        g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Victim.szName, victim, sizeof( victim ) );
        g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Killer.szName, killer, sizeof( killer ) );

        // Get the local position for this notice
        int len = UTIL_ComputeStringWidth( m_hTextFont, victim );
        int y = yStart + (m_flLineHeight * i);

        int iconWide;
        int iconTall;

        if( icon->bRenderUsingFont )
        {
            iconWide = surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
            iconTall = surface()->GetFontTall( icon->hFont );
        }
        else
        {
            float scale = ( (float)ScreenHeight() / 480.0f );	//scale based on 640x480
            iconWide = (int)( scale * (float)icon->Width() );
            iconTall = (int)( scale * (float)icon->Height() );
        }

        int x;
        if ( m_bRightJustify )
        {
            x =	GetWide() - len - iconWide;
        }
        else
        {
            x = 0;
        }
        
        // Only draw killers name if it wasn't a suicide
        if ( !m_DeathNotices[i].bAloneDeath )
        {
            if ( m_bRightJustify )
            {
                x -= UTIL_ComputeStringWidth( m_hTextFont, killer );
            }

            SetColorForNoticePlayer( m_DeathNotices[i].Killer.iTeam );

            // Draw killer's name
            surface()->DrawSetTextPos( x, y );
            surface()->DrawSetTextFont( m_hTextFont );
            surface()->DrawUnicodeString( killer );
            surface()->DrawGetTextPos( x, y );
        }

        Color iconColor( 255, 80, 0, 255 );

        // Draw death weapon
        //If we're using a font char, this will ignore iconTall and iconWide
        icon->DrawSelf( x, y, iconWide, iconTall, iconColor );
        x += iconWide;		

        SetColorForNoticePlayer( m_DeathNotices[i].Victim.iTeam );

        // Draw victims name
        surface()->DrawSetTextPos( x, y );
        surface()->DrawSetTextFont( m_hTextFont );	//reset the font, draw icon can change it
        surface()->DrawUnicodeString( victim );
    }

    // Now retire any death notices that have expired
    RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::RetireExpiredDeathNotices()
{
    // Loop backwards because we might remove one
    int iSize = m_DeathNotices.Size();
    for ( int i = iSize-1; i >= 0; i-- )
    {
        if ( m_DeathNotices[i].flDisplayTime < gpGlobals->curtime )
        {
            m_DeathNotices.Remove(i);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CZMHudDeathNotice::FireGameEvent( IGameEvent* event )
{
    if ( !g_PR )
        return;

    if ( hud_deathnotice_time.GetFloat() == 0 )
        return;

    // the event should be "player_death"
    int killer = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
    int victim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
    const char* killedwith = event->GetString( "weapon" );


    char fullkilledwith[128];
    if ( killedwith && *killedwith )
    {
        Q_snprintf( fullkilledwith, sizeof( fullkilledwith ), "death_%s", killedwith );
    }
    else
    {
        fullkilledwith[0] = 0;
    }


    int iKillerTeam = 0;
    int iVictimTeam = 0;

    if ( victim )
    {
        iVictimTeam = g_PR->GetTeam( victim );
    }

    if ( killer )
    {
        iKillerTeam = g_PR->GetTeam( killer );
    }


    // Do we have too many death messages in the queue?
    if ( m_DeathNotices.Count() > 0 &&
        m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
    {
        // Remove the oldest one in the queue, which will always be the first
        m_DeathNotices.Remove( 0 );
    }

    // Get the names of the players
    const char *killer_name = g_PR->GetPlayerName( killer );
    const char *victim_name = g_PR->GetPlayerName( victim );

    if ( !killer_name || !*killer_name || Q_strcmp( killer_name, PLAYER_ERROR_NAME ) == 0 )
    {
        killer_name = "";


        if ( fullkilledwith[0] != NULL )
        {
            // ZMRTODO: Put this somewhere else.
            if ( Q_strcmp( &fullkilledwith[6], "zombie" ) == 0 ) killer_name = "Shambler";
            else if ( Q_strcmp( &fullkilledwith[6], "fastzombie" ) == 0 ) killer_name = "Banshee";
            else if ( Q_strcmp( &fullkilledwith[6], "poisonzombie" ) == 0 ) killer_name = "Hulk";
            else if ( Q_strcmp( &fullkilledwith[6], "dragzombie" ) == 0 )  killer_name = "Drifter";
            else if ( Q_strcmp( &fullkilledwith[6], "burnzombie" ) == 0 ) killer_name = "Immolator";


            if ( killer_name[0] != NULL )
            {
                iKillerTeam = ZMTEAM_ZM;
            }
        }
    }


    if ( !victim_name || !*victim_name || Q_strcmp( victim_name, PLAYER_ERROR_NAME ) == 0 )
        victim_name = "";



    bool bAloneDeath = ( killer == victim || (!killer && killer_name[0] == NULL) );


    // Make a new death notice
    DeathNoticeItem deathMsg;
    deathMsg.Killer.iTeam = iKillerTeam;
    deathMsg.Victim.iTeam = iVictimTeam;
    Q_strncpy( deathMsg.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH );
    Q_strncpy( deathMsg.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH );
    deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
    deathMsg.bAloneDeath = bAloneDeath;

    // Try and find the death identifier in the icon list
    deathMsg.iconDeath = gHUD.GetIcon( fullkilledwith );

    if ( !deathMsg.iconDeath || bAloneDeath )
    {
        // Can't find it, so use the default skull & crossbones icon
        deathMsg.iconDeath = m_iconD_skull;
    }

    // Add it to our list of death notices
    m_DeathNotices.AddToTail( deathMsg );



    // Record the death notice in the console
    char szMsg[256];

    if ( bAloneDeath )
    {
        if ( killer == victim )
        {
            Q_snprintf( szMsg, sizeof( szMsg ), "%s suicided.\n", victim_name );
        }
        else
        {
            Q_snprintf( szMsg, sizeof( szMsg ), "%s died.\n", victim_name );
        }
        
    }
    else
    {
        Q_snprintf( szMsg, sizeof( szMsg ), "%s killed %s", killer_name, victim_name );

        if ( fullkilledwith && *fullkilledwith && iKillerTeam != ZMTEAM_ZM )
        {
            Q_strncat( szMsg, VarArgs( " with %s.\n", &fullkilledwith[6] ), sizeof( szMsg ), COPY_ALL_CHARACTERS );
        }
        else
        {
            Q_strncat( szMsg, ".\n", sizeof( szMsg ), COPY_ALL_CHARACTERS );
        }
    }

    Msg( "%s", szMsg );
}
