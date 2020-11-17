#include "cbase.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "zmr_gamerules.h"
#include "zmr_hud_centertext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


extern ConVar zm_sv_roundintermissiontime;

ConVar zm_cl_round_cinematic_effects_text( "zm_cl_round_cinematic_effects_text", "1", FCVAR_ARCHIVE, "Center text" );
ConVar zm_cl_round_cinematic_effects_duration( "zm_cl_round_cinematic_effects_duration", "5", FCVAR_ARCHIVE, "The cinematic effects duration." );


CON_COMMAND( zm_hudcentertext, "Displays a center text message." )
{
    CZMHudCenterText* txt = GET_HUDELEMENT( CZMHudCenterText );

    if ( txt )
    {
        const char* pszDelay = args.Arg( 3 );
        const char* pszDisplay = args.Arg( 4 );

        float smalldelay = ( pszDelay && *pszDelay ) ? atoi( pszDelay ) : 0.0f;
        float displaytime = ( pszDisplay && *pszDisplay ) ? atoi( pszDisplay ) : 5.0f;

        txt->ShowText( args.Arg( 1 ), args.Arg( 2 ), smalldelay, displaytime );
    }
}


DECLARE_HUDELEMENT( CZMHudCenterText );
DECLARE_HUD_MESSAGE( CZMHudCenterText, ZMCenterText );


CZMHudCenterText::CZMHudCenterText( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudCenterText" )
{
    SetPaintBackgroundEnabled( false );


    Reset_();


    ListenForGameEvent( "round_end_post" );
    ListenForGameEvent( "round_restart_post" );
}

CZMHudCenterText::~CZMHudCenterText()
{
    
}

void CZMHudCenterText::Init()
{
    HOOK_HUD_MESSAGE( CZMHudCenterText, ZMCenterText );

    Reset_();
}

void CZMHudCenterText::VidInit()
{
    Reset_();
}

// Reset() is called on player spawn
// We don't like that.
void CZMHudCenterText::Reset_()
{
    SetWide( ScreenWidth() );


    m_szBig[0] = NULL;
    m_szSmall[0] = NULL;
    m_flShowSmall = 0.0f;
    m_flNextHide = 0.0f;
}

void CZMHudCenterText::FireGameEvent( IGameEvent* pEvent )
{
    //
    // Round start/end effects
    //
    if ( Q_strcmp( pEvent->GetName(), "round_end_post" ) == 0 )
    {
        ZMRoundEndReason_t reason = (ZMRoundEndReason_t)pEvent->GetInt( "reason", ZMROUND_GAMEBEGIN );

        ShowRoundEnd( reason );
    }
    else if ( Q_strcmp( pEvent->GetName(), "round_restart_post" ) == 0 )
    {
        ShowRoundStart();
    }
}

void CZMHudCenterText::OnThink()
{
    if ( m_flNextHide != 0.0f )
    {
        if ( m_flNextHide <= gpGlobals->curtime )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeOut" );

            m_flNextHide = 0.0f;
        }

        if ( m_flShowSmall != 0.0f && m_flShowSmall <= gpGlobals->curtime )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeInSmall" );

            m_flShowSmall = 0.0f;
        }
    }
}

void CZMHudCenterText::PaintString( const HFont font, const HFont shadowfont, const Color& clr, int x, int y, const wchar_t* txt )
{
	surface()->DrawSetTextPos( x, y );
    surface()->DrawSetTextColor( Color( 0, 0, 0, clr[3] ) );
    surface()->DrawSetTextFont( shadowfont );
	surface()->DrawUnicodeString( txt );

    surface()->DrawSetTextPos( x, y );
    surface()->DrawSetTextColor( clr );
    surface()->DrawSetTextFont( font );
	surface()->DrawUnicodeString( txt );
}

void CZMHudCenterText::Paint()
{
    if ( m_flBigAlpha <= 0.0f && m_flSmallAlpha <= 0.0f ) return;


    int h_big;
    int w, h;
    surface()->GetTextSize( m_hBigFont, m_szBig, w, h_big );

    if ( m_szBig[0] != NULL )
    {
        m_Color[3] = m_flBigAlpha;
        PaintString( m_hBigFont, m_hBigShadowFont, m_Color, GetWide() / 2.0f - w / 2.0f, m_flBigPosY, m_szBig );
    }

    if ( m_szSmall[0] != NULL )
    {
        surface()->GetTextSize( m_hSmallFont, m_szSmall, w, h );

        m_Color[3] = m_flSmallAlpha;
        PaintString( m_hSmallFont, m_hSmallShadowFont, m_Color, GetWide() / 2.0f - w / 2.0f, m_flBigPosY + h_big + m_flSmallOffsetY, m_szSmall );
    }
}

void CZMHudCenterText::ShowRoundStart( const char* insmalltxt )
{
    if ( !zm_cl_round_cinematic_effects_text.GetBool() )
    {
        return;
    }


    //CZMHudCenterText* center = GET_HUDELEMENT( CZMHudCenterText );

    //if ( !center ) return;


    CZMRules* pRules = ZMRules();
    


    wchar_t txt[128];
    wchar_t smalltxt[128];
    txt[0] = NULL;
    smalltxt[0] = NULL;

    wchar_t wround[64];
    wround[0] = NULL;
    if ( pRules )
    {
        // This is just silly...
        char round[32];
        Q_snprintf( round, sizeof( round ), "%i", pRules->GetRoundsPlayed() + 1 );
        g_pVGuiLocalize->ConvertANSIToUnicode( round, wround, sizeof( wround ) );
    }

    g_pVGuiLocalize->ConstructString( txt, sizeof( txt ), g_pVGuiLocalize->Find( "#ZMRoundCount" ), 1, wround );


    if ( insmalltxt && *insmalltxt )
    {
        // It's a localized text.
        if ( *insmalltxt == '#' )
        {
            Q_wcsncpy( smalltxt, g_pVGuiLocalize->Find( insmalltxt ), sizeof( smalltxt ) );
        }
        // Just copy over.
        else
        {
            g_pVGuiLocalize->ConvertANSIToUnicode( insmalltxt, smalltxt, sizeof( smalltxt ) );
        }
    }


    ShowText( txt, smalltxt, 1.0f, zm_cl_round_cinematic_effects_duration.GetFloat() );
}

void CZMHudCenterText::ShowRoundEnd( ZMRoundEndReason_t reason )
{
    if ( !zm_cl_round_cinematic_effects_text.GetBool() )
    {
        return;
    }


    const char* localized = nullptr;
    switch ( reason )
    {
    case ZMROUND_GAMEBEGIN : localized = "#ZMRoundEndShortGameBegin"; break;
    case ZMROUND_HUMANDEAD : localized = "#ZMRoundEndShortHumanDead"; break;
    case ZMROUND_HUMANLOSE : localized = "#ZMRoundEndShortHumanLose"; break;
    case ZMROUND_HUMANWIN : localized = "#ZMRoundEndShortHumanWin"; break;
    case ZMROUND_VOTERESTART : localized = "#ZMRoundEndShortVoteRestart"; break;
    case ZMROUND_ZMSUBMIT : localized = "#ZMRoundEndShortSubmit"; break;
    default : localized = "#ZMRoundEndShortGeneric"; break;
    }

    // Add a bit of padding in case of lag.
    ShowText( localized, nullptr, 0.0f, zm_sv_roundintermissiontime.GetFloat() + 0.5f );
}

void CZMHudCenterText::ShowText( const char* bigtxt, const char* smalltxt, float smalldelay, float displaytime )
{
    wchar_t big[256];
    wchar_t small[256];

    big[0] = NULL;
    small[0] = NULL;

    if ( bigtxt )
    {
        if ( bigtxt[0] == '#' )
        {
            const wchar_t* pTemp = g_pVGuiLocalize->Find( bigtxt );

            if ( pTemp )
            {
                Q_wcsncpy( big, pTemp, sizeof( big ) );
            }
        }

        if ( big[0] == NULL )
            g_pVGuiLocalize->ConvertANSIToUnicode( bigtxt, big, sizeof( big ) );
    }
    
    if ( smalltxt )
    {
        if ( smalltxt[0] == '#' )
        {
            const wchar_t* pTemp = g_pVGuiLocalize->Find( smalltxt );

            if ( pTemp )
            {
                Q_wcsncpy( small, pTemp, sizeof( small ) );
            }
        }

        if ( small[0] == NULL)
            g_pVGuiLocalize->ConvertANSIToUnicode( smalltxt, small, sizeof( small ) );
    }
    

    ShowText( big, small, smalldelay, displaytime );
}

void CZMHudCenterText::ShowText( const wchar_t* bigtxt, const wchar_t* smalltxt, float smalldelay, float displaytime )
{
    if ( bigtxt != nullptr )
    {
        Q_wcsncpy( m_szBig, bigtxt, sizeof( m_szBig ) );
    }

    if ( smalltxt != nullptr )
    {
        Q_wcsncpy( m_szSmall, smalltxt, sizeof( m_szSmall ) );
    }

    if ( bigtxt || smalltxt )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeIn" );

        m_flNextHide = gpGlobals->curtime + displaytime;


        m_flShowSmall = gpGlobals->curtime + smalldelay;
    }
    else
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMCenterTextFadeOut" );
    }
}

void CZMHudCenterText::MsgFunc_ZMCenterText( bf_read& msg )
{
    char buffer[256];
    wchar_t big[256];
    wchar_t small[256];


    msg.ReadString( buffer, sizeof( buffer ), true );
    g_pVGuiLocalize->ConvertANSIToUnicode( buffer, big, sizeof( big ) );

    msg.ReadString( buffer, sizeof( buffer ), true );
    g_pVGuiLocalize->ConvertANSIToUnicode( buffer, small, sizeof( small ) );


    ShowText( big, small );
}

