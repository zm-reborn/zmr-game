#include "cbase.h"


#include "iclientmode.h"
#include "hud_macros.h"
#include <vgui/ILocalize.h>
#include <engine/IEngineSound.h>

#include "usermessages.h"
//#include "c_user_message_register.h"


#include "IGameUIFuncs.h"
#include <igameresources.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern IGameUIFuncs* gameuifuncs; // for key binding details



#include "zmr/c_zmr_util.h"
#include "zmr_hud_tooltips.h"


using namespace vgui;



ConVar zm_cl_showhelp( "zm_cl_showhelp", "1", FCVAR_ARCHIVE, "Show help?" );
ConVar zm_cl_help_randomtips( "zm_cl_help_randomtips", "320", FCVAR_ARCHIVE, "Show random tips every X seconds. 0 = disable" );



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



CZMHudTooltip::CZMHudTooltip( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudTooltip" )
{
    SetPaintBackgroundEnabled( false );


    m_vQueue.Purge();

    m_nTexId = 0;
    m_nTexSize = 0;

    m_flNextRandomTip = 0.0f;
    m_flNextHide = 0.0f;
    m_bPulse = false;
    m_iPriority = 0;


    m_pTextImage = new TextImage( "" );
    m_pTextImage->SetWrap( true );
}

CZMHudTooltip::~CZMHudTooltip()
{
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


    auto* pToolTips = g_ZMTipSystem.GetManagerByIndex( ZMTIPMANAGER_TOOLTIPS );
    Assert( pToolTips );

    float curtime = gpGlobals->curtime;

    int len = m_vQueue.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vQueue[i].m_flDisplay > curtime ) continue;


        tip = pToolTips->FindTipByIndex( m_vQueue[i].m_iIndex );

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

    if ( zm_cl_showhelp.GetBool() )
        return;


    auto* pToolTips = g_ZMTipSystem.GetManagerByIndex( ZMTIPMANAGER_TOOLTIPS );
    Assert( pToolTips );

    ZMConstTips_t vTips;
    pToolTips->GetTips( vTips );

    for ( int i = 0; i < vTips.Count(); i++ )
    {
        if ( !vTips[i]->ShowRandom() || !vTips[i]->ShouldShow() )
        {
            vTips.Remove( i );
            --i;
        }
    }

    if ( vTips.Count() )
    {
        QueueTip( vTips[random->RandomInt( 0, vTips.Count() - 1)]->GetIndex(), 0.0f );
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

void CZMHudTooltip::QueueTip( const char* name, float delay )
{
    auto* pToolTips = g_ZMTipSystem.GetManagerByIndex( ZMTIPMANAGER_TOOLTIPS );
    Assert( pToolTips );

    auto* pTip = pToolTips->FindTipByName( name );
    if ( pTip )
    {
        QueueTip( pTip, delay );
    }
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

int CZMHudTooltip::SetMessageByName( const char* name, bool force )
{
    auto* pToolTips = g_ZMTipSystem.GetManagerByIndex( ZMTIPMANAGER_TOOLTIPS );
    Assert( pToolTips );


    CZMTip* tip = pToolTips->FindTipByName( name );

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
        tip->OnShownInGame();
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
