#include "cbase.h"
//#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
//#include "ihudlcd.h"
#include "in_buttons.h"
#include "c_user_message_register.h"


#include "zmr/c_zmr_player.h"
#include "zmr/zmr_shareddefs.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




using namespace vgui;


#define OBJ_SCREEN_TIME             7.0f

#define OBJ_FADE_RATE               512.0f

#define TRANSITION_NONE             0
#define TRANSITION_HIDE             ( 1 << 0 )
#define TRANSITION_SHOW             ( 1 << 1 )
#define TRANSITION_CHANGETEXT       ( 1 << 2 )


class CObjLine
{
public:
    ObjArgType_t GetArgType( int i ) { return m_iArgType; };


    void Reset()
    {
        m_szRenderText[0] = NULL;
        m_szTexts[0] = NULL;
        m_szFormat[0] = NULL;

        m_iArgType = OBJARGTYPE_NONE;
        m_szArg[0] = NULL;
        m_flArg = 0.0f;
        m_Color = COLOR_WHITE;

        m_flWantedPosY = 0.0f;
        m_flAbsPosY = 0.0f;

        m_fTransition = TRANSITION_NONE;
        m_nAlpha = 0;
    }

    void UpdateComplete()
    {
        SetComplete( false );
        UpdateRenderText();
    }

    void UpdateRenderText()
    {
        if ( m_szTexts[0] == NULL )
        {
            m_szRenderText[0] = NULL;
            return;
        }


        g_pVGuiLocalize->ConvertANSIToUnicode( m_szTexts, m_szRenderText, sizeof( m_szRenderText ) );
    }

    void SetArgs( const char* format, ObjArgType_t type, float num, const char* str )
    {
        Q_strncpy( m_szFormat, ( format && *format ) ? format : "", sizeof( m_szFormat ) );
        m_iArgType = type;
        
        SetArg( num, str );
    }

    void SetArg( float num, const char* str )
    {
        m_flArg = num;
        Q_strncpy( m_szArg, ( str && *str ) ? str : "", sizeof( m_szArg ) );
    }

    void SetComplete( bool state )
    {
        if ( state )
        {
            m_Color = COLOR_GREEN;
        }
        else
        {
            m_Color = COLOR_WHITE;
        }

        m_bComplete = state;
    }

    void FormatNextLine()
    {
        char szBuffer[256];


        if ( m_szFormat[0] )
        {
            switch ( m_iArgType )
            {
            case OBJARGTYPE_FLOAT :
                Q_snprintf( szBuffer, sizeof( szBuffer ), m_szFormat, m_flArg );
                break;
            case OBJARGTYPE_STRING :
                Q_snprintf( szBuffer, sizeof( szBuffer ), m_szFormat, m_szArg );
                break;
            case OBJARGTYPE_TIMER :
            case OBJARGTYPE_INT :
                Q_snprintf( szBuffer, sizeof( szBuffer ), m_szFormat, (int)m_flArg );
                break;
    
            default : Q_strncpy( szBuffer, m_szFormat, sizeof( szBuffer ) ); break;
            }
        }
        else
        {
            szBuffer[0] = 0;
        }

        Q_strncpy( m_szTexts, szBuffer, sizeof( m_szTexts ) );
    }

    void Hide()
    {
        m_fTransition |= TRANSITION_HIDE;
    }

    void Show()
    {
        m_fTransition |= TRANSITION_SHOW;
    }

    bool IsInTransition()
    {
        return m_fTransition & (TRANSITION_HIDE | TRANSITION_CHANGETEXT | TRANSITION_SHOW) ? true : false;
    }

    void SetTransition( bool bShow )
    {
        m_fTransition = TRANSITION_HIDE | TRANSITION_CHANGETEXT;

        if ( bShow )
        {
            m_fTransition |= TRANSITION_SHOW;
        }
    }

    void DoTransition()
    {
        if ( m_fTransition == TRANSITION_NONE && IsEmpty() && HasNextText() )
        {
            SetTransition( true );
        }


        int rate = 0;

        if ( m_fTransition & (TRANSITION_HIDE | TRANSITION_CHANGETEXT) )
        {
            rate = -OBJ_FADE_RATE * gpGlobals->frametime;
            if ( rate == 0 ) rate = -1;
        }
        else if ( m_fTransition & TRANSITION_SHOW )
        {
            rate = OBJ_FADE_RATE * gpGlobals->frametime;
            if ( rate == 0 ) rate = 1;
        }


        m_nAlpha = clamp( m_nAlpha + rate, 0, 255 );


        if ( m_nAlpha <= 0 )
        {
            m_fTransition &= ~TRANSITION_HIDE;


            if ( m_fTransition & TRANSITION_CHANGETEXT )
            {
                UpdateComplete();
                m_fTransition &= ~TRANSITION_CHANGETEXT;

                if ( IsEmpty() )
                    m_fTransition &= ~TRANSITION_SHOW;
            }
        }
        else if ( m_nAlpha >= 255 )
        {
            m_fTransition &= ~TRANSITION_SHOW;
        }
    }

    bool Paint( HFont font, int x, int &y )
    {
        if ( m_szRenderText[0] == NULL )
            return false;

        if ( m_nAlpha > 0 )
        {
            m_Color[3] = m_nAlpha;

	        surface()->DrawSetTextFont( font );
            surface()->DrawSetTextColor( m_Color );
	        surface()->DrawSetTextPos( x, y + m_flAbsPosY );
	        surface()->DrawUnicodeString( m_szRenderText );
        }

        int w, h;
        surface()->GetTextSize( font, L"O", w, h );
        y += h;

        return true;
    }

    bool IsEmpty()
    {
        return m_szRenderText[0] == NULL;
    }

    bool IsHidden()
    {
        return m_nAlpha <= 0;
    }

    bool HasNextText()
    {
        return m_szTexts[0] != NULL;
    }

private:
    wchar_t m_szRenderText[256];
    char m_szTexts[128];
    char m_szFormat[128];
    ObjArgType_t m_iArgType;
    float m_flArg;
    char m_szArg[64];
    Color m_Color;

    bool m_bComplete;

    float m_flAbsPosY;
    float m_flWantedPosY;


    int m_fTransition;
    int m_nAlpha;
};




class CZMHudObjectives : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudObjectives, Panel );

    CZMHudObjectives( const char *pElementName );
    ~CZMHudObjectives();


    virtual void Init( void ) OVERRIDE;
    virtual void VidInit( void ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint( void ) OVERRIDE;


    void MsgFunc_ZMObjDisplay( bf_read& msg );
    void MsgFunc_ZMObjUpdate( bf_read& msg );
    void MsgFunc_ZMObjUpdateLine( bf_read& msg );
    

    void InitTexts();
    void SetNextText( int i, const char* format, ObjArgType_t, float num, const char* psz );
    void Update( int i, float num, const char* psz, bool bComplete );
    void Transition( int i = -1 );
    void SetRecipients( ObjRecipient_t );

    void AutoDraw( bool bNewObjective );
    bool ShouldAutoDraw();

    bool IsHidden()
    {
        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
            if ( !m_Line[i].IsHidden() )
                return false;
        
        return true;
    };

    bool IsEmpty()
    {
        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
            if ( !m_Line[i].IsEmpty() )
                return false;
        
        return true;
    }

    bool HasNextText()
    {
        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
            if ( !m_Line[i].HasNextText() )
                return false;

        return true;
    }

private:
    void Hide();
    void Show();
    void SetTransition( bool bShow = true );
    void UpdateComplete();



    CPanelAnimationVar( HFont, m_hMainFont, "MainFont", "ZMHudObjectives" );
    CPanelAnimationVar( HFont, m_hChildFont, "ChildFont", "ZMHudObjectivesSmall" );

    CPanelAnimationVarAliasType( float, m_MainPosX, "MainPosX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_MainPosY, "MainPosY", "0", "proportional_float" );


    float m_flLastShow;
    ObjRecipient_t m_iRecipient;
    CObjLine m_Line[NUM_OBJ_LINES];
};

DECLARE_HUDELEMENT( CZMHudObjectives );
DECLARE_HUD_MESSAGE( CZMHudObjectives, ZMObjDisplay );
DECLARE_HUD_MESSAGE( CZMHudObjectives, ZMObjUpdate );
DECLARE_HUD_MESSAGE( CZMHudObjectives, ZMObjUpdateLine );


CZMHudObjectives::CZMHudObjectives( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudObjectives" )
{
    InitTexts();

    m_flLastShow = 0.0f;
    m_iRecipient = OBJRECIPIENT_INVALID;
}

CZMHudObjectives::~CZMHudObjectives()
{
}

void CZMHudObjectives::Init( void )
{
    HOOK_HUD_MESSAGE( CZMHudObjectives, ZMObjDisplay );
    HOOK_HUD_MESSAGE( CZMHudObjectives, ZMObjUpdate );
    HOOK_HUD_MESSAGE( CZMHudObjectives, ZMObjUpdateLine );


    SetPaintBackgroundEnabled( false );
}

void CZMHudObjectives::VidInit( void )
{
    // This makes sure changing resolutions, etc. won't reset the text while in-game.
    // This still resets when connecting to a server, etc.
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( pPlayer ) return;


    InitTexts();
}

void CZMHudObjectives::Reset()
{
}

void CZMHudObjectives::OnThink()
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer ) return;


    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        m_Line[i].DoTransition();


    if ( pPlayer->m_nButtons & IN_SCORE && !(pPlayer->m_afButtonLast & IN_SCORE) )
    {
        Show();
    }

    if ( (gpGlobals->curtime - m_flLastShow) > OBJ_SCREEN_TIME )
    {
        Hide();
    }
}

void CZMHudObjectives::InitTexts()
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
    {
        m_Line[i].Reset();
    }
}

void CZMHudObjectives::SetNextText( int i, const char* format, ObjArgType_t type, float num, const char* psz )
{
    if ( i < 0 || i >= NUM_OBJ_LINES ) return;


    m_Line[i].SetArgs( format, type, num, psz );
    m_Line[i].FormatNextLine();
    
    if ( IsHidden() )
    {
        m_Line[i].UpdateComplete();
    }
}

void CZMHudObjectives::Update( int i, float num, const char* psz, bool bComplete )
{
    if ( i < 0 || i >= NUM_OBJ_LINES ) return;


    m_Line[i].SetComplete( bComplete );
    m_Line[i].SetArg( num, psz );
    m_Line[i].FormatNextLine();
    m_Line[i].UpdateRenderText();
}

bool CZMHudObjectives::ShouldAutoDraw()
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer ) return false;


    if ( pPlayer->IsZM() && m_iRecipient == OBJRECIPIENT_HUMANS )
    {
        return false;
    }

    return true;
}

void CZMHudObjectives::AutoDraw( bool bNewObjective )
{
    if ( !ShouldAutoDraw() )
        Show();
}

void CZMHudObjectives::Transition( int i )
{
    if ( IsHidden() || i == -1 )
    {
        SetTransition( ShouldAutoDraw() );
    }
    else
    {
        m_Line[i].SetTransition( ShouldAutoDraw() );
    }

    m_flLastShow = gpGlobals->curtime;
}

void CZMHudObjectives::SetRecipients( ObjRecipient_t rec )
{
    m_iRecipient = rec;
}

void CZMHudObjectives::Hide()
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        m_Line[i].Hide();
}

void CZMHudObjectives::Show()
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        m_Line[i].Show();

    m_flLastShow = gpGlobals->curtime;
}

void CZMHudObjectives::SetTransition( bool bShow )
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        m_Line[i].SetTransition( bShow );
}

void CZMHudObjectives::UpdateComplete()
{
    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        m_Line[i].UpdateComplete();
}

void CZMHudObjectives::Paint()
{
    int y = m_MainPosY;
    m_Line[0].Paint( m_hMainFont, m_MainPosX, y );

    for ( int i = 1; i < NUM_OBJ_LINES; i++ )
    {
        if ( m_Line[i].Paint( m_hChildFont, m_MainPosX, y ) )
            y -= 6;
    }
}

void CZMHudObjectives::MsgFunc_ZMObjDisplay( bf_read& msg )
{
    char arg[32];
    char format[256];


    SetRecipients( (ObjRecipient_t)msg.ReadByte() );


    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
    {
        ObjArgType_t argtype = OBJARGTYPE_NONE;
        float num = 0.0f;
        arg[0] = NULL;


        bool bString = msg.ReadByte() == 1 ? true : false;

        argtype = (ObjArgType_t)msg.ReadByte();


        switch ( argtype )
        {
        case OBJARGTYPE_STRING : msg.ReadString( arg, sizeof( arg ), false ); break;
        case OBJARGTYPE_TIMER :
        case OBJARGTYPE_INT :
        case OBJARGTYPE_FLOAT : num = msg.ReadFloat(); break;
        default : break;
        }

        
        if ( bString )
            msg.ReadString( format, sizeof( format ), false );
        else
            format[0] = 0;


        SetNextText( i, format, argtype, num, arg );
    }

    Transition();
}

void CZMHudObjectives::MsgFunc_ZMObjUpdate( bf_read& msg )
{
    char buffer[32];

    for ( int i = 0; i < NUM_OBJ_LINES; i++ )
    {
        float num = 0.0f;
        buffer[0] = NULL;


        bool bComplete = msg.ReadByte() == 1 ? true : false;
        ObjArgType_t type = (ObjArgType_t)msg.ReadByte();

        if ( type == OBJARGTYPE_STRING )
        {
            msg.ReadString( buffer, sizeof( buffer ), false );
        }
        else
        {
            num = msg.ReadFloat();
        }

        Update( i, num, buffer, bComplete );
    }

    AutoDraw( false );
}

void CZMHudObjectives::MsgFunc_ZMObjUpdateLine( bf_read& msg )
{
    char arg[32];
    char format[256];


    int i = msg.ReadByte();

    if ( i < 0 || i >= NUM_OBJ_LINES ) return;


    SetRecipients( (ObjRecipient_t)msg.ReadByte() );


    ObjArgType_t argtype = OBJARGTYPE_NONE;
    float num = 0.0f;
    arg[0] = NULL;


    bool bString = msg.ReadByte() == 1 ? true : false;

    argtype = (ObjArgType_t)msg.ReadByte();


    switch ( argtype )
    {
    case OBJARGTYPE_STRING : msg.ReadString( arg, sizeof( arg ), false ); break;
    case OBJARGTYPE_TIMER :
    case OBJARGTYPE_INT :
    case OBJARGTYPE_FLOAT : num = msg.ReadFloat(); break;
    default : break;
    }

        
    if ( bString )
        msg.ReadString( format, sizeof( format ), false );
    else
        format[0] = 0;


    SetNextText( i, format, argtype, num, arg );

    Transition( i );
}
