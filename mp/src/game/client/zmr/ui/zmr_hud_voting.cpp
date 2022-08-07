#include "cbase.h"

#include "hudelement.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "usermessages.h"
//#include "c_user_message_register.h"

#include "IGameUIFuncs.h"
#include <igameresources.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern IGameUIFuncs* gameuifuncs; // for key binding details


using namespace vgui;


class CZMHudVote : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudVote, Panel );

    CZMHudVote( const char* pElementName );
    ~CZMHudVote();


    virtual void Init( void ) OVERRIDE;
    virtual void LevelInit() OVERRIDE;
    virtual void VidInit( void ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;
    virtual void FireGameEvent( IGameEvent* event ) OVERRIDE;

    void MsgFunc_CallVoteFailed( bf_read &msg );
    void MsgFunc_VoteStart( bf_read &msg );
    void MsgFunc_VotePass( bf_read &msg );
    void MsgFunc_VoteFailed( bf_read &msg );
    void MsgFunc_VoteSetup( bf_read &msg );

private:
    void InitVoting();
    void PaintString( const HFont, const Color&, int x, int y, const wchar_t* );

    bool m_bDrawVote;
    float m_flVoteDraw;

    const wchar_t* m_pszPass;
    const wchar_t* m_pszFail;
    const wchar_t* m_pszMyVote;
    wchar_t m_szHowto[128];
    wchar_t m_szDisplay[128];
    wchar_t m_szVote[32];

    const wchar_t* m_pszReason;


    CPanelAnimationVar( HFont, m_hVoteFont, "VoteFont", "ZMHudVote" );
    CPanelAnimationVar( Color, m_VoteColor, "VoteColor", "0 0 0 0" );

    CPanelAnimationVar( HFont, m_hReasonFont, "ReasonFont", "ZMHudVoteReason" );
    CPanelAnimationVar( Color, m_ReasonColor, "ReasonColor", "0 0 0 0" );

    CPanelAnimationVar( HFont, m_hTextFont, "TextFont", "ZMHudVoteText" );
};

DECLARE_HUDELEMENT( CZMHudVote );
DECLARE_HUD_MESSAGE( CZMHudVote, CallVoteFailed );
DECLARE_HUD_MESSAGE( CZMHudVote, VoteStart );
DECLARE_HUD_MESSAGE( CZMHudVote, VotePass );
DECLARE_HUD_MESSAGE( CZMHudVote, VoteFailed );
DECLARE_HUD_MESSAGE( CZMHudVote, VoteSetup );


CZMHudVote::CZMHudVote( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudVote" )
{
    m_bDrawVote = false;
    m_flVoteDraw = 0.0f;
    
    m_szDisplay[0] = NULL;
    m_szHowto[0] = NULL;
    m_szVote[0] = NULL;

    m_pszMyVote = nullptr;
    m_pszReason = nullptr;
    m_pszPass = nullptr;
    m_pszFail = nullptr;

    SetPaintBackgroundEnabled( false );
    SetProportional( false );
}

CZMHudVote::~CZMHudVote()
{
    
}

void CZMHudVote::Init( void )
{
    HOOK_HUD_MESSAGE( CZMHudVote, CallVoteFailed );
    HOOK_HUD_MESSAGE( CZMHudVote, VoteStart );
    HOOK_HUD_MESSAGE( CZMHudVote, VotePass );
    HOOK_HUD_MESSAGE( CZMHudVote, VoteFailed );
    HOOK_HUD_MESSAGE( CZMHudVote, VoteSetup );

    ListenForGameEvent( "vote_cast" );
    ListenForGameEvent( "vote_changed" );

    InitVoting();
}

void CZMHudVote::LevelInit()
{
    m_bDrawVote = false;

    InitVoting();
}

void CZMHudVote::VidInit( void )
{
    InitVoting();
}

void CZMHudVote::Reset()
{
    // IMPORTANT!!! Animation variables must be reset here.
    // All animations are removed on Reset.
    // If there are still animations going, they'll be frozen.
    m_bDrawVote = false;

    InitVoting();
}

void CZMHudVote::InitVoting()
{
    // Have to set this here since we can't use proportional.
    SetWide( ScreenWidth() );

    SetPos( 0, ScreenHeight() - GetTall() - ScreenHeight() * 0.06f );


    wchar_t szYes[16];
    wchar_t szNo[16];

    const char* yes = KeyCodeToString( gameuifuncs->GetButtonCodeForBind( "Vote Option1" ) );
    const char* no = KeyCodeToString( gameuifuncs->GetButtonCodeForBind( "Vote Option2" ) );

    g_pVGuiLocalize->ConvertANSIToUnicode( *yes ? &yes[4] : "(UNBOUND)", szYes, sizeof( szYes ) );
    g_pVGuiLocalize->ConvertANSIToUnicode( *no ? &no[4] : "(UNBOUND)", szNo, sizeof( szNo ) );

    g_pVGuiLocalize->ConstructString( m_szHowto, sizeof( m_szHowto ), g_pVGuiLocalize->Find( "#ZMVoteHowto" ), 2,
        szYes, szNo );


    m_pszPass = g_pVGuiLocalize->Find( "#ZMVotePassed" );
    m_pszFail = g_pVGuiLocalize->Find( "#ZMVoteFailed" );

    if ( !m_bDrawVote )
    {
        m_VoteColor[3] = 0;
        m_ReasonColor[3] = 0;
    }
}

void CZMHudVote::FireGameEvent( IGameEvent* event )
{
    const char* name = event->GetName();

    if ( FStrEq( name, "vote_cast" ) )
    {
        int option = event->GetInt( "vote_option" );
        //int team = event->GetInt( "team" );
        int ent = event->GetInt( "entityid" );

        if ( GetLocalPlayerIndex() == ent )
        {
            m_pszMyVote = g_pVGuiLocalize->Find( ( option == 1 ) ?  "#ZMMyVoteYes" : "#ZMMyVoteNo" );
        }
    }
    else if ( FStrEq( name, "vote_changed" ) )
    {
        int i = 0;
        //for ( int i = 0; i < MAX_VOTE_OPTIONS; i++ )
        //{	
            char szOption[2];
            Q_snprintf( szOption, sizeof( szOption ), "%i", i + 1 );

            char szVoteOption[13] = "vote_option";
            Q_strncat( szVoteOption, szOption, sizeof( szVoteOption ), COPY_ALL_CHARACTERS );

            int yesvotes = event->GetInt( szVoteOption );
        //}

        int potential = event->GetInt( "potentialVotes" );


        V_snwprintf( m_szVote, ARRAYSIZE( m_szVote ), L"%i/%i", yesvotes, potential );
    }
}

void CZMHudVote::OnThink()
{
    if ( m_bDrawVote && m_flVoteDraw < gpGlobals->curtime )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMVoteEnd" );
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMVoteReasonEnd" );

        m_bDrawVote = false;
    }
}

void CZMHudVote::PaintString( const HFont font, const Color& clr, int x, int y, const wchar_t* txt )
{
	surface()->DrawSetTextFont( font );
    surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextPos( x, y );
	surface()->DrawUnicodeString( txt );
}

void CZMHudVote::Paint()
{
    if ( m_VoteColor[3] <= 0 ) return;

    // Using GetWide instead of ScreenWidth since it's easier to notice if things mismatch.
    int w, h;

    if ( m_ReasonColor[3] > 0 && m_pszReason && *m_pszReason )
    {
        surface()->GetTextSize( m_hReasonFont, m_pszReason, w, h );

        PaintString( m_hReasonFont, m_ReasonColor, GetWide() / 2.0f - w / 2.0f, 10, m_pszReason );
    }


    if ( m_szDisplay[0] != NULL )
    {
        surface()->GetTextSize( m_hVoteFont, m_szDisplay, w, h );

        PaintString( m_hVoteFont, m_VoteColor, GetWide() / 2.0f - w / 2.0f, 40, m_szDisplay );
    }


    if ( m_szHowto[0] != NULL )
    {
        surface()->GetTextSize( m_hTextFont, m_szHowto, w, h );
        PaintString( m_hTextFont, m_VoteColor, GetWide() / 2.0f - w / 2.0f, 60, m_szHowto );
    }


    if ( m_szVote[0] != NULL )
    {
        surface()->GetTextSize( m_hTextFont, m_szVote, w, h );

        PaintString( m_hTextFont, m_VoteColor, GetWide() / 2.0f - w / 2.0f, 80, m_szVote );
    }

    if ( m_pszMyVote && *m_pszMyVote )
    {
        surface()->GetTextSize( m_hTextFont, m_pszMyVote, w, h );

        PaintString( m_hTextFont, m_VoteColor, GetWide() / 2.0f - w / 2.0f, 100 + 1 + h * 2, m_pszMyVote );
    }
}

void CZMHudVote::MsgFunc_VoteSetup( bf_read& msg )
{
    int count = msg.ReadByte();
    DevMsg( "VoteSetup | Issue Count: %i\n", count );
    char type[64];
    char type_local[64];
    bool bEnabled;
    
    for ( int i = 0; i < count; i++ )
    {
        msg.ReadString( type, sizeof( type ) );
        msg.ReadString( type_local, sizeof( type_local ) );
        bEnabled = msg.ReadByte();

        DevMsg( "Type: %s | Local: %s\n", type, type_local );
    }
}

void CZMHudVote::MsgFunc_VoteStart( bf_read& msg )
{
    char display[256];
    char details[64];

    msg.ReadByte();
    msg.ReadByte();
    msg.ReadString( display, sizeof( display ) );
    msg.ReadString( details, sizeof( details ) );


    DevMsg( "VoteStart | Display: %s | Details: %s\n", display, details );


    const wchar_t* local = nullptr;

    if ( display[0] == L'#' )
    {
        local = g_pVGuiLocalize->Find( display );
    }
    
    if ( local && *local )
    {
        V_wcsncpy( m_szDisplay, local, sizeof( m_szDisplay ) );
    }
    else
    {
        g_pVGuiLocalize->ConvertANSIToUnicode( display, m_szDisplay, sizeof( m_szDisplay ) );
    }
    


    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMVoteStart" );

    m_szVote[0] = NULL;
    m_pszMyVote = nullptr;
    m_pszReason = nullptr;
    m_bDrawVote = true;
    m_flVoteDraw = gpGlobals->curtime + 60.0f;
}

void CZMHudVote::MsgFunc_CallVoteFailed( bf_read& msg )
{
    DevMsg( "CallVoteFailed | Reason: %i | Time: %i\n", msg.ReadByte(), msg.ReadShort() );


    m_flVoteDraw = gpGlobals->curtime + 3.0f;
}

void CZMHudVote::MsgFunc_VoteFailed( bf_read& msg )
{
    DevMsg( "VoteFailed | Only Team: %i | Reason: %i\n", msg.ReadByte(), msg.ReadByte() );
    

    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMVoteReasonStart" );

    m_pszReason = m_pszFail;
    m_flVoteDraw = gpGlobals->curtime + 6.0f;
}

void CZMHudVote::MsgFunc_VotePass( bf_read& msg )
{
    char passed[64];
    char details[64];

    msg.ReadString( passed, sizeof( passed ) );
    msg.ReadString( details, sizeof( details ) );

    DevMsg( "VotePass | Passed: %s | Details: %s\n", passed, details );



    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMVoteReasonStart" );

    m_pszReason = m_pszPass;
    m_flVoteDraw = gpGlobals->curtime + 6.0f;
}
