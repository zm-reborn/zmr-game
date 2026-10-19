#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode.h"
#include "baseviewport.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_baseplayer.h"
#include "c_team.h"


#include "zmr_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;



static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

ConVar zm_cl_deathnotice_enabled( "zm_cl_deathnotice_enabled", "1", FCVAR_ARCHIVE, "Are deathnotices displayed?" );
ConVar zm_cl_deathnotice_console_enabled( "zm_cl_deathnotice_console_enabled", "1", FCVAR_ARCHIVE, "Are deathnotices displayed in console?" );



struct DeathNoticePlayer
{
    wchar_t wszName[MAX_PLAYER_NAME_LENGTH];
    int     iTeam;
};

struct DeathNoticeItem 
{
    DeathNoticePlayer	Killer;
    DeathNoticePlayer   Victim;
    bool                bAloneDeath;
    float               flCreatedTime;
};

class CZMHudDeathNotice : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE( CZMHudDeathNotice, Panel );
public:
    CZMHudDeathNotice( const char* pElementName );

    void Init() OVERRIDE;
    void VidInit() OVERRIDE;
    virtual bool ShouldDraw() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;
    virtual void ApplySchemeSettings( IScheme* scheme ) OVERRIDE;

    void RetireExpiredDeathNotices();
    
    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;

private:

    CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "15", "proportional_float" );
    CPanelAnimationVar( int, m_nMaxDeathNotices, "MaxDeathNotices", "6" );
    CPanelAnimationVar( float, m_flDeathNoticeFadeInTime, "DeathNoticeFadeInTime", "0.4" );
    CPanelAnimationVar( float, m_flDeathNoticeFadeOutTime, "DeathNoticeFadeOutTime", "0.8" );
    CPanelAnimationVar( HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );


    // Texture for skull symbol
    CHudTexture* m_iconD_skull;
    CUtlVector<DeathNoticeItem> m_DeathNotices;
    int m_iSkullTextureId;

    float m_flOffset;

    int GetCurrentAlpha( const DeathNoticeItem& item, float displayTime ) const;
};

DECLARE_HUDELEMENT( CZMHudDeathNotice );

CZMHudDeathNotice::CZMHudDeathNotice( const char* pElementName ) :
    CHudElement( pElementName ), BaseClass( nullptr, "ZMHudDeathNotice" )
{
    SetParent( g_pClientMode->GetViewport() );

    m_flLineHeight = 0.0f;
    m_nMaxDeathNotices = 0;
    m_flDeathNoticeFadeInTime = 0.0f;
    m_flDeathNoticeFadeOutTime = 0.0f;
    m_hTextFont = INVALID_FONT;
    m_iconD_skull = nullptr;
    m_flOffset = 0.0f;

    SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_iSkullTextureId = surface()->DrawGetTextureId( "vgui/zmr_deathnotice/generic" );
	if ( m_iSkullTextureId == -1 )
	{
		m_iSkullTextureId = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( m_iSkullTextureId, "vgui/zmr_deathnotice/generic", true, false );
	}
}

void CZMHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
    BaseClass::ApplySchemeSettings( scheme );
    SetPaintBackgroundEnabled( false );
}

void CZMHudDeathNotice::Init()
{
    ListenForGameEvent( "player_death" );	
}

void CZMHudDeathNotice::VidInit()
{
    m_iconD_skull = gHUD.GetIcon( "d_skull" );
    m_DeathNotices.Purge();
}

bool CZMHudDeathNotice::ShouldDraw()
{
    return CHudElement::ShouldDraw() && ( m_DeathNotices.Count() || m_flOffset > 0.0f );
}

void CZMHudDeathNotice::OnThink()
{
    m_flOffset -= gpGlobals->frametime * 250.0f;
    if ( m_flOffset < 0.0f )
    {
        m_flOffset = 0.0f;
    }

    RetireExpiredDeathNotices();
}

void CZMHudDeathNotice::Paint()
{
    if ( !g_PR )
    {
        return;
    }

    float displayTime = hud_deathnotice_time.GetFloat();

    int yStart = YRES( 16 );

    surface()->DrawSetTextFont( m_hTextFont );
    surface()->DrawSetTextColor( g_PR->GetTeamColor( 0 ) );

    int iCount = m_DeathNotices.Count();
    for ( int i = 0; i < iCount; i++ )
    {
        const DeathNoticeItem& item = m_DeathNotices[i];

        // Get the local position for this notice
        int textWidth;
        int textHeight;
        surface()->GetTextSize( m_hTextFont, item.Victim.wszName, textWidth, textHeight );

        int y = yStart + m_flOffset + (m_flLineHeight * i);

        float scale = ScreenHeight() / 480.0f;	//scale based on 640x480
        int iconWide = scale * 16;
        int iconTall = scale * 16;

        int x = GetWide() - textWidth - iconWide;

        int alpha = GetCurrentAlpha( item, displayTime );
        
        // Only draw killers name if it wasn't a suicide
        if ( !item.bAloneDeath )
        {
            x -= UTIL_ComputeStringWidth( m_hTextFont, item.Killer.wszName );

            // Draw killer's name
            Color color = g_PR->GetTeamColor( item.Killer.iTeam );
            color[3] = alpha;
            surface()->DrawSetTextColor( color );
            surface()->DrawSetTextPos( x, y );
            surface()->DrawSetTextFont( m_hTextFont );
            surface()->DrawUnicodeString( item.Killer.wszName );
            surface()->DrawGetTextPos( x, y );
        }

        if ( m_iSkullTextureId != -1 )
        {
            // Center image
            int iconY = y + textHeight / 2.0f - iconTall / 2.0f;

            Color iconColor( 255, 255, 255, alpha );

            surface()->DrawSetColor( iconColor );
            surface()->DrawSetTexture( m_iSkullTextureId );
            surface()->DrawTexturedRect( x, iconY, x + iconWide, iconY + iconTall );
        }

        x += iconWide;

        // Draw victims name
        Color color = g_PR->GetTeamColor( item.Victim.iTeam );
        color[3] = alpha;
        surface()->DrawSetTextColor( color );
        surface()->DrawSetTextPos( x, y );
        surface()->DrawSetTextFont( m_hTextFont );	//reset the font, draw icon can change it
        surface()->DrawUnicodeString( item.Victim.wszName );
    }
}

int CZMHudDeathNotice::GetCurrentAlpha( const DeathNoticeItem& item, float displayTime ) const
{
    int alpha = 255;

    float aliveTime = gpGlobals->curtime - item.flCreatedTime;
    if ( m_flDeathNoticeFadeInTime > 0.0f && aliveTime < m_flDeathNoticeFadeInTime )
    {
        alpha = Lerp( aliveTime / m_flDeathNoticeFadeInTime, 0, 255 );
    }
    
    float timeleft = item.flCreatedTime + displayTime - gpGlobals->curtime;
    if ( m_flDeathNoticeFadeOutTime > 0.0f && timeleft < m_flDeathNoticeFadeOutTime )
    {
        alpha = Lerp( timeleft / m_flDeathNoticeFadeOutTime, 0, 255 );
    }

    return alpha;
}

void CZMHudDeathNotice::RetireExpiredDeathNotices()
{
    float displayTime = hud_deathnotice_time.GetFloat();

    // Loop backwards because we might remove one
    int iSize = m_DeathNotices.Size();
    for ( int i = iSize-1; i >= 0; i-- )
    {
        const DeathNoticeItem& item = m_DeathNotices[i];
        if ( item.flCreatedTime + displayTime < gpGlobals->curtime )
        {
            m_DeathNotices.Remove(i);
            m_flOffset += m_flLineHeight;
        }
    }
}

void CZMHudDeathNotice::FireGameEvent( IGameEvent* event )
{
    if ( !g_PR )
    {
        return;
    }

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
        m_DeathNotices.Count() >= m_nMaxDeathNotices )
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


        if ( fullkilledwith[0] != '\0' )
        {
            // ZMRTODO: Put this somewhere else.
            if ( Q_strcmp( &fullkilledwith[6], "zombie" ) == 0 ) killer_name = "Shambler";
            else if ( Q_strcmp( &fullkilledwith[6], "fastzombie" ) == 0 ) killer_name = "Banshee";
            else if ( Q_strcmp( &fullkilledwith[6], "poisonzombie" ) == 0 ) killer_name = "Hulk";
            else if ( Q_strcmp( &fullkilledwith[6], "dragzombie" ) == 0 )  killer_name = "Drifter";
            else if ( Q_strcmp( &fullkilledwith[6], "burnzombie" ) == 0 ) killer_name = "Immolator";

            if ( killer_name[0] != '\0' )
            {
                iKillerTeam = ZMTEAM_ZM;
            }
        }
    }


    if ( !victim_name || !*victim_name || Q_strcmp( victim_name, PLAYER_ERROR_NAME ) == 0 )
    {
        victim_name = "";
    }


    bool bAloneDeath = ( killer == victim || (!killer && killer_name[0] == '\0') );

    float displayTime = hud_deathnotice_time.GetFloat();

    // Make a new death notice
    DeathNoticeItem deathMsg;
    deathMsg.Killer.iTeam = iKillerTeam;
    deathMsg.Victim.iTeam = iVictimTeam;
    g_pVGuiLocalize->ConvertANSIToUnicode( killer_name, deathMsg.Killer.wszName, sizeof( deathMsg.Killer.wszName ) );
    g_pVGuiLocalize->ConvertANSIToUnicode( victim_name, deathMsg.Victim.wszName, sizeof( deathMsg.Victim.wszName ) );
    deathMsg.flCreatedTime = gpGlobals->curtime;
    deathMsg.bAloneDeath = bAloneDeath;

    // Add it to our list of death notices
    if ( zm_cl_deathnotice_enabled.GetBool() && displayTime > 0.0f )
    {
        m_DeathNotices.AddToTail( deathMsg );
    }


    // Record the death notice in the console
    if ( zm_cl_deathnotice_console_enabled.GetBool() )
    {
        char szMsg[256];

        if ( bAloneDeath )
        {
            if ( killer == victim )
            {
                Q_snprintf( szMsg, sizeof( szMsg ), "%s suicided.", victim_name );
            }
            else
            {
                Q_snprintf( szMsg, sizeof( szMsg ), "%s died.", victim_name );
            }
            
        }
        else
        {
            Q_snprintf( szMsg, sizeof( szMsg ), "%s killed %s", killer_name, victim_name );

            if ( fullkilledwith[0] != '\0' && iKillerTeam != ZMTEAM_ZM )
            {
                Q_strncat( szMsg, VarArgs( " with %s.", &fullkilledwith[6] ), sizeof( szMsg ), COPY_ALL_CHARACTERS );
            }
            else
            {
                Q_strncat( szMsg, ".", sizeof( szMsg ), COPY_ALL_CHARACTERS );
            }
        }

        Msg( "%s\n", szMsg );
    }
}
