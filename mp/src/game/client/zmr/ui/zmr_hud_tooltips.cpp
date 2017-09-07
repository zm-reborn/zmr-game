#include "cbase.h"


#include "iclientmode.h"
#include "hud_macros.h"
#include "filesystem.h"
#include <vgui/ILocalize.h>
#include <engine/IEngineSound.h>

#include "usermessages.h"
//#include "c_user_message_register.h"

#include "zmr/zmr_util.h"
#include "zmr_hud_tooltips.h"


using namespace vgui;


#define DEF_TIP_TEAM            0
#define DEF_TIP_TIME            5.0f
#define DEF_TIP_PRIORITY        0


DECLARE_HUDELEMENT( CZMHudTooltip );
DECLARE_HUD_MESSAGE( CZMHudTooltip, ZMTooltip );



CON_COMMAND( zm_hudtooltip, "Displays a tool-tip message." )
{
    ZMClientUtil::ShowTooltip( args.ArgS() );
}

CON_COMMAND( zm_hudtooltipbyname, "Displays a tool-tip by name." )
{
    ZMClientUtil::ShowTooltipByName( args.ArgS() );
}

CZMTip::CZMTip( KeyValues* kv, int index )
{
    m_iIndex = index;


    m_pszName = nullptr;
    m_pszMessage = nullptr;
    m_pszIcon = nullptr;
    Reset();


    m_iTeam = kv->GetInt( "team", DEF_TIP_TEAM );
    m_flTime = kv->GetFloat( "time", DEF_TIP_TIME );
    m_iPriority = kv->GetInt( "priority", DEF_TIP_PRIORITY );
    m_bQueue = kv->GetBool( "canbequeued", false );
    m_bPulse = kv->GetBool( "pulse", false );
    m_bSound = kv->GetBool( "sound", false );
    SetName( kv->GetName() );
    SetMessage( kv->GetString( "msg" ) );
    SetIcon( kv->GetString( "icon" ) );
}

CZMTip::~CZMTip()
{
    delete[] m_pszName;
    delete[] m_pszMessage;
    delete[] m_pszIcon;
}

void CZMTip::Reset()
{
    m_iTeam = DEF_TIP_TEAM;
    m_flTime = DEF_TIP_TIME;
    m_iPriority = DEF_TIP_PRIORITY;
    m_bQueue = false;
    m_bPulse = false;
    m_bSound = false;
}

const char* CZMTip::GetName()
{
    return m_pszName;
}

void CZMTip::SetName( const char* name )
{
    delete[] m_pszName;

    if ( !name ) return;


    int len = strlen( name ) + 1;
    m_pszName = new char[len];
    Q_strncpy( m_pszName, name, len );
}

const char* CZMTip::GetMessage()
{
    return m_pszMessage;
}

void CZMTip::SetMessage( const char* msg )
{
    delete[] m_pszMessage;

    if ( !msg ) return;


    int len = strlen( msg ) + 1;
    m_pszMessage = new char[len];
    Q_strncpy( m_pszMessage, msg, len );
}

const char* CZMTip::GetIcon()
{
    return m_pszIcon;
}

void CZMTip::SetIcon( const char* icon )
{
    delete[] m_pszIcon;

    if ( !icon ) return;


    int len = strlen( icon ) + 1;
    m_pszIcon = new char[len];
    Q_strncpy( m_pszIcon, icon, len );
}

CZMHudTooltip::CZMHudTooltip( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudTooltip" )
{
    SetPaintBackgroundEnabled( false );


    m_vTips.Purge();
    m_vQueue.Purge();

    m_nTexId = 0;

    m_flNextHide = 0.0f;
    m_bPulse = false;
    m_iPriority = 0;


    m_pTextImage = new TextImage( "" );
    m_pTextImage->SetWrap( true );
}

CZMHudTooltip::~CZMHudTooltip()
{
    m_vTips.PurgeAndDeleteElements();

    delete[] m_pTextImage;
}

void CZMHudTooltip::Init()
{
    HOOK_HUD_MESSAGE( CZMHudTooltip, ZMTooltip );



    m_vTips.PurgeAndDeleteElements();

    KeyValues* kv = new KeyValues( "Tips" );
    kv->UsesEscapeSequences( true );


    if ( kv->LoadFromFile( filesystem, "resource/zmtips.txt", "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();

        while ( pKey )
        {
            AddTip( pKey );
            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load tips from file!\n" );
    }

    kv->deleteThis();


    Reset();
    HideTooltip();
}

void CZMHudTooltip::LevelInit()
{
    Reset();
    HideTooltip();
}

void CZMHudTooltip::VidInit()
{
    Reset();
    HideTooltip();
}

void CZMHudTooltip::Reset()
{
    SetWide( ScreenWidth() );
}

void CZMHudTooltip::OnThink()
{
    if ( m_flNextHide != 0.0f || m_flAlphaMult == 1.0f )
    {
        if ( m_flNextHide <= gpGlobals->curtime || !CanDisplay() )
            HideTooltip();
    }
}

void CZMHudTooltip::Paint()
{
    if ( m_flAlphaMult <= 0.0f ) return;

    if ( !m_pTextImage->GetUText() || m_pTextImage->GetUText()[0] == NULL )
        return;

#define PADDING_SIDES       6
#define PADDING_TOP         5
#define PADDING_IMAGE       4


    
    int w, h;
    m_pTextImage->GetContentSize( w, h );


    int tex_size = !m_nTexId ? 0 : ( h * 1.333f );
    int padding_image = !tex_size ? 0 : PADDING_IMAGE;



    int content_size = w + tex_size + padding_image;

    int box_w = content_size + PADDING_SIDES * 2;
    int box_h = max( h, tex_size ) + PADDING_TOP * 2;




    int box_x = GetWide() / 2.0f - box_w / 2.0f;

    int y = YRES( 420 );

    DrawBox( box_x, y, box_w, box_h, m_BgColor, m_flAlphaMult );

    int content_x = (GetWide() / 2.0f - content_size / 2.0f);
    int tex_y = y + ((box_h - tex_size) / 2.0f);
    int text_y = y + ((box_h - h) / 2.0f);




    int pulse;

    if ( m_bPulse )
    {
        pulse = max( 128, 255 * sin( gpGlobals->curtime * 7.0f ) );
    }
    else
    {
        pulse = 255;
    }


    Color clr( 255, pulse, pulse, m_flAlphaMult * 255 );


    if ( tex_size > 0 )
    {
        surface()->DrawSetColor( clr );
        surface()->DrawSetTexture( m_nTexId );
        surface()->DrawTexturedRect( content_x, tex_y, content_x + tex_size, tex_y + tex_size );
    }


    m_pTextImage->SetColor( clr );
    m_pTextImage->SetPos( content_x + padding_image + tex_size, text_y );
    m_pTextImage->Paint();
}

void CZMHudTooltip::AddTip( KeyValues* kv )
{
    static int index = 1;

    CZMTip* tip = new CZMTip( kv, index );

    m_vTips.AddToTail( tip );


    ++index;
}

bool CZMHudTooltip::CanDisplay()
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer ) return false;


    if ( m_iTeam != 0 && pPlayer->GetTeamNumber() != m_iTeam )
    {
        return false;
    }

    return true;
}

void CZMHudTooltip::HideTooltip()
{
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMTooltipHide" );
    
    m_iCurIndex = 0;

    m_flNextHide = 0.0f;
}

CZMTip* CZMHudTooltip::FindMessageByName( const char* name )
{
    int len = m_vTips.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( Q_stricmp( name, m_vTips[i]->GetName() ) == 0 )
        {
            return m_vTips[i];
        }
    }

    return nullptr;
}

int CZMHudTooltip::SetMessageByName( const char* name )
{
    CZMTip* tip = FindMessageByName( name );

    if ( !tip ) return 0;


    SetMessage( tip->GetMessage(), tip->GetIndex(), tip->GetTime(), tip->GetPulse(), tip->GetPriority(), tip->GetIcon(), tip->DoSound(), tip->GetTeam() );

    return tip->GetIndex();
}

void CZMHudTooltip::SetMessage( const char* msg, int index, float displaytime, bool pulse, int priority, const char* image, bool bSound, int team )
{
    if ( !msg ) return;

    if ( IsDisplayingTip() && !CanBeOverriden( priority ) )
    {
        return;
    }


    m_iTeam = team;

    if ( !CanDisplay() ) return;


    SetText( msg );


    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMTooltipShow" );


    m_flNextHide = gpGlobals->curtime + displaytime;
    m_bPulse = pulse;
    m_iPriority = priority;


    if ( image && *image )
    {
        int id = surface()->DrawGetTextureId( image );

        if ( id < 1 )
        {
            id = surface()->CreateNewTextureID();
            surface()->DrawSetTextureFile( id, image, true, false );
        }

        m_nTexId = id;
    }
    else
    {
        m_nTexId = 0;
    }

    if ( bSound )
    {
        CLocalPlayerFilter filter;
        C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "ZMTooltip.Show" );
    }

    m_iCurIndex = index;
}

void CZMHudTooltip::SetText( const char* txt )
{
    wchar_t wtxt[512];
    wchar_t* pTxt = nullptr;

    
    if ( txt[0] == '#' )
    {
        pTxt = g_pVGuiLocalize->Find( txt );
    }

    if ( !pTxt || !*pTxt )
    {
        g_pVGuiLocalize->ConvertANSIToUnicode( txt, wtxt, sizeof( wtxt ) );
        pTxt = wtxt;
    }
    
    


    int max_w = GetWide() * 0.6f;

    int w, h;
    surface()->GetTextSize( m_hFont, pTxt, w, h );

    if ( w > max_w ) w = max_w;

    m_pTextImage->SetSize( w, 100 );
    m_pTextImage->SetText( pTxt );
    m_pTextImage->SetFont( m_hFont );
}

void CZMHudTooltip::MsgFunc_ZMTooltip( bf_read& msg )
{
    int priority = msg.ReadByte();
    float displaytime = msg.ReadFloat();
    bool pulse = msg.ReadByte() ? true : false;
    bool sound = msg.ReadByte() ? true : false;

    char display[256];
    char img[128];
    msg.ReadString( display, sizeof( display ) );
    msg.ReadString( img, sizeof( img ) );

    SetMessage( display, 0, displaytime, pulse, priority, img, sound );
}
