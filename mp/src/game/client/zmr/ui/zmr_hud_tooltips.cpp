#include "cbase.h"


#include "iclientmode.h"
#include "hud_macros.h"
#include "filesystem.h"
#include <vgui/ILocalize.h>
#include <engine/IEngineSound.h>

#include "usermessages.h"
//#include "c_user_message_register.h"


#include "IGameUIFuncs.h"
#include <igameresources.h>

extern IGameUIFuncs* gameuifuncs; // for key binding details



#include "zmr/c_zmr_util.h"
#include "zmr_hud_tooltips.h"


using namespace vgui;



ConVar zm_cl_showhelp( "zm_cl_showhelp", "1", FCVAR_ARCHIVE, "Show help?" );
ConVar zm_cl_help_randomtips( "zm_cl_help_randomtips", "320", FCVAR_ARCHIVE, "Show random tips every X seconds. 0 = disable" );



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

CON_COMMAND( zm_hudtooltipsaveshown, "Saves shown tool-tips to a file." )
{
    CZMHudTooltip* tips = GET_HUDELEMENT( CZMHudTooltip );

    if ( tips )
    {
        return tips->SaveUsed();
    }
}

CZMTip::CZMTip( KeyValues* kv, int index )
{
    m_iIndex = index;


    m_pszName = nullptr;
    m_pszMessage = nullptr;
    m_pszIcon = nullptr;
    m_pszParam = nullptr;
    Reset();


    m_iTeam = kv->GetInt( "team", DEF_TIP_TEAM );
    m_flTime = kv->GetFloat( "time", DEF_TIP_TIME );
    m_iPriority = kv->GetInt( "priority", DEF_TIP_PRIORITY );
    m_nLimit = kv->GetInt( "limit" );
    m_nLimitPerGame = kv->GetInt( "limitpergame" );
    m_bQueue = kv->GetBool( "canbequeued", false );
    m_bRandom = kv->GetBool( "random", false );
    m_bPulse = kv->GetBool( "pulse", false );
    m_bSound = kv->GetBool( "sound", false );
    SetName( kv->GetName() );
    SetMessage( kv->GetString( "msg" ) );
    SetIcon( kv->GetString( "icon" ) );
    SetParam( kv->GetString( "param1" ) );
}

void CZMTip::WriteUsed( KeyValues* kv )
{
    if ( m_nLimit < 1 ) return;


    KeyValues* pKey = kv->FindKey( m_pszName, true );

    pKey->SetInt( "shown", m_nShown );
}

void CZMTip::LoadUsed( KeyValues* kv )
{
    m_nShown = kv->GetInt( "shown" );
}

CZMTip::~CZMTip()
{
    delete[] m_pszName;
    delete[] m_pszMessage;
    delete[] m_pszIcon;
    delete[] m_pszParam;
}

void CZMTip::Reset()
{
    m_iTeam = DEF_TIP_TEAM;
    m_flTime = DEF_TIP_TIME;
    m_iPriority = DEF_TIP_PRIORITY;
    m_bQueue = false;
    m_bRandom = false;
    m_bPulse = false;
    m_bSound = false;

    m_nShown = 0;
    m_nShownPerGame = 0;
}

bool CZMTip::ShouldShow()
{
    if ( m_nLimit > 0 && m_nShown >= m_nLimit ) return false;

    if ( m_nLimitPerGame > 0 && m_nShownPerGame >= m_nLimitPerGame ) return false;


    if ( (m_nLimit > 0 || m_nLimitPerGame > 0) && !zm_cl_showhelp.GetBool() )
        return false;
    

    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();

    if ( m_iTeam != DEF_TIP_TEAM )
    {
        if ( !pLocal || pLocal->GetTeamNumber() != m_iTeam )
            return false;
    }


    return true;
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

void CZMTip::FormatMessage( char* buffer, int len )
{
    if ( !m_pszMessage ) return;


    char param[128];
    param[0] = NULL;

    switch ( m_iParamType )
    {
    case TIPPARAMTYPE_KEY :
    {
        const char* key = Panel::KeyCodeToString( gameuifuncs->GetButtonCodeForBind( m_pszParam ) );

        Q_strncpy( param, ( key && *key ) ? &key[4] : "(UNBOUND)", sizeof( param ) );

        break;
    }
    case TIPPARAMTYPE_CVAR :
    {
        ConVar* pCon = cvar->FindVar( m_pszParam );

        if ( pCon )
            Q_strncpy( param, pCon->GetString(), sizeof( param ) );

        break;
    }
    default : break;
    }



    if ( param )
    {
        const char* pFormat = m_pszMessage;

        if ( pFormat[0] == '#' )
        {
            pFormat = g_pVGuiLocalize->FindAsUTF8( m_pszMessage );
        }

        if ( !pFormat )
            pFormat = m_pszMessage;


        Q_snprintf( buffer, len, pFormat, param );
    }
    else
    {
        Q_strncpy( buffer, m_pszMessage, len );
    }
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

void CZMTip::SetParam( const char* invalue )
{
    m_iParamType = TIPPARAMTYPE_NONE;
    delete[] m_pszParam;


    if ( !invalue ) return;


    char buffer[128];
    Q_strncpy( buffer, invalue, sizeof( buffer ) );


    char* del = strchr( buffer, '@' );

    if ( !del ) return;


    *del = NULL;

    char* param = del + 1;


    m_iParamType = TipNameToType( buffer );

    int len = strlen( param ) + 1;
    m_pszParam = new char[len];
    Q_strncpy( m_pszParam, param, len );
}

TipParamType_t CZMTip::TipNameToType( const char* name )
{
    if ( Q_stricmp( name, "key" ) == 0 )
    {
        return TIPPARAMTYPE_KEY;
    }
    else if ( Q_stricmp( name, "cvar" ) == 0 )
    {
        return TIPPARAMTYPE_CVAR;
    }

    return TIPPARAMTYPE_NONE;
}

CZMHudTooltip::CZMHudTooltip( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudTooltip" )
{
    SetPaintBackgroundEnabled( false );


    m_vTips.Purge();
    m_vQueue.Purge();

    m_nTexId = 0;
    m_nTexSize = 0;

    m_flNextRandomTip = 0.0f;
    m_flNextHide = 0.0f;
    m_bPulse = false;
    m_iPriority = 0;


    m_pTextImage = new TextImage( "" );
    m_pTextImage->SetWrap( true );


    LoadTips();
    LoadUsed();
}

CZMHudTooltip::~CZMHudTooltip()
{
    m_vTips.PurgeAndDeleteElements();

    delete[] m_pTextImage;
}

void CZMHudTooltip::Init()
{
    HOOK_HUD_MESSAGE( CZMHudTooltip, ZMTooltip );


    Reset();
    HideTooltip();
}

void CZMHudTooltip::LevelInit()
{
    Reset();
    HideTooltip();

    m_vQueue.Purge();
}

void CZMHudTooltip::LevelShutdown()
{
    SaveUsed();
}

void CZMHudTooltip::VidInit()
{
    Reset();
    HideTooltip();
}

void CZMHudTooltip::Reset()
{
    SetWide( ScreenWidth() );

    m_flNextSound = 0.0f;
}

void CZMHudTooltip::OnThink()
{
    if ( IsDisplayingTip() && m_flNextHide != 0.0f )
    {
        if ( m_flNextHide <= gpGlobals->curtime || !CanDisplay() )
            HideTooltip();
    }
    
    if ( !IsDisplayingTip() )
    {
        if ( m_vQueue.Count() )
        {
            FindNextQueueTip();
        }
        else
        {
            FindNextRandomTipToQueue();
        }
    }
}

void CZMHudTooltip::FindNextQueueTip()
{
    // Find highest priority queued tip.

    CZMTip* highest_tip = nullptr;
    int highest_index = -1;

    CZMTip* tip;


    float curtime = gpGlobals->curtime;

    int len = m_vQueue.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vQueue[i].m_flDisplay > curtime ) continue;


        tip = FindMessageByIndex( m_vQueue[i].m_iIndex );

        if ( !tip ) continue;


        if ( highest_tip == nullptr || highest_tip->GetPriority() < tip->GetPriority() )
        {
            highest_tip = tip;
            highest_index = i;
        }
    }
        

    if ( highest_tip )
    {
        SetMessageByName( highest_tip->GetName() );

        m_vQueue.Remove( highest_index );
    }
}

void CZMHudTooltip::FindNextRandomTipToQueue()
{
    float delay = zm_cl_help_randomtips.GetFloat();

    if ( delay == 0.0f )
        return;

    if ( m_flNextRandomTip > gpGlobals->curtime )
        return;


    CUtlVector<int> vTips;
    vTips.Purge();

    int len = m_vTips.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vTips[i]->ShowRandom() && m_vTips[i]->ShouldShow() )
        {
            vTips.AddToTail( m_vTips[i]->GetIndex() );
        }
    }

    if ( vTips.Count() )
    {
        QueueTip( vTips[random->RandomInt( 0, vTips.Count() - 1)], 0.0f );
    }


    m_flNextRandomTip = gpGlobals->curtime + delay;
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


    int tex_size = !m_nTexId ? 0 : m_nTexSize;
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
        pulse = max( 128, 255 * sin( gpGlobals->curtime * 5.0f ) );
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
    // Need to stop it immediately.
    g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( g_pClientMode->GetViewportAnimationController()->GetParent(), "ZMTooltipShow" );
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMTooltipHide" );
    
    m_iCurIndex = 0;

    m_flNextHide = gpGlobals->curtime + 2.0f;
}


void CZMHudTooltip::QueueTip( int index, float delay )
{
    ZMTipQueue_T queue;
    queue.m_iIndex = index;
    queue.m_flDisplay = gpGlobals->curtime + delay;

    m_vQueue.AddToTail( queue );
}

void CZMHudTooltip::QueueTip( CZMTip* tip, float delay )
{
    QueueTip( tip->GetIndex(), delay );
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

CZMTip* CZMHudTooltip::FindMessageByIndex( int index )
{
    int len = m_vTips.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( index == m_vTips[i]->GetIndex() )
        {
            return m_vTips[i];
        }
    }

    return nullptr;
}

int CZMHudTooltip::SetMessageByName( const char* name, bool force )
{
    CZMTip* tip = FindMessageByName( name );

    if ( !tip ) return 0;


    if ( !force && !tip->ShouldShow() )
    {
        return tip->GetIndex();
    }


    char buffer[512];
    buffer[0] = NULL;

    tip->FormatMessage( buffer, sizeof( buffer ) );

    bool res = SetMessage( buffer, tip->GetIndex(), tip->GetTime(), tip->GetPulse(), tip->GetPriority(), tip->GetIcon(), tip->DoSound(), tip->GetTeam() );


    if ( res )
    {
        tip->IncShown();
    }
    else
    {
        if ( tip->CanBeQueued() )
            QueueTip( tip, 0.0f );
    }

    return tip->GetIndex();
}

bool CZMHudTooltip::SetMessage( const char* msg, int index, float displaytime, bool pulse, int priority, const char* image, bool bSound, int team )
{
    if ( !msg ) return false;

    if ( IsDisplayingTip() && !CanBeOverriden( priority ) )
    {
        return false;
    }


    m_iTeam = team;

    if ( !CanDisplay() ) return false;


    SetText( msg );

    // Need to stop it immediately.
    g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( g_pClientMode->GetViewportAnimationController()->GetParent(), "ZMTooltipHide" );
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

    if ( bSound && m_flNextSound <= gpGlobals->curtime )
    {
        CLocalPlayerFilter filter;
        C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "ZMTooltip.Show" );


        m_flNextSound = gpGlobals->curtime + displaytime + 6.0f;
    }

    m_iCurIndex = index;

    return true;
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

    m_nTexSize = h * 1.333f;


    if ( w > max_w ) w = max_w;

    m_pTextImage->SetSize( w, 100 );
    m_pTextImage->SetText( pTxt );
    m_pTextImage->SetFont( m_hFont );
}

void CZMHudTooltip::LoadTips()
{
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
}

void CZMHudTooltip::LoadUsed()
{
    DevMsg( "Loading used tips...\n" );


    KeyValues* kv = new KeyValues( "Tips" );

    if ( kv->LoadFromFile( filesystem, "resource/zmtips_used.txt", "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();

        while ( pKey )
        {
            CZMTip* tip = FindMessageByName( pKey->GetName() );


            if ( tip )
            {
                tip->LoadUsed( pKey );
            }

            pKey = pKey->GetNextKey();
        }
    }

    kv->deleteThis();
}

void CZMHudTooltip::SaveUsed()
{
    DevMsg( "Saving used tips...\n" );


    KeyValues* kv = new KeyValues( "Tips" );


    for ( int i = 0; i < m_vTips.Count(); i++ )
    {
        m_vTips[i]->WriteUsed( kv );
    }


    if ( m_vTips.Count() )
    {
        kv->SaveToFile( filesystem, "resource/zmtips_used.txt", "MOD" );
    }
    

    kv->deleteThis();
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
