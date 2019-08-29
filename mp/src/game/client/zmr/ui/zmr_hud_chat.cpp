#include "cbase.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "ihudlcd.h"

#include "zmr_shareddefs.h"
#include "zmr/c_zmr_importancesystem.h"

#include "zmr_hud_chat.h"



DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );


//=====================
//CHudChatLine
//=====================

void CHudChatLine::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );
}

//=====================
//CHudChatInputLine
//=====================

void CHudChatInputLine::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );
}

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char* pElementName ) : BaseClass( pElementName )
{
    
}

void CHudChat::CreateChatInputLine( void )
{
    m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
    m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
    m_ChatLine = new CHudChatLine( this, "ChatLine1" );
    m_ChatLine->SetVisible( false );	
}

void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );
}


void CHudChat::Init( void )
{
    BaseClass::Init();

    HOOK_HUD_MESSAGE( CHudChat, SayText );
    HOOK_HUD_MESSAGE( CHudChat, SayText2 );
    HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: Overrides base reset to not cancel chat at round restart
//-----------------------------------------------------------------------------
void CHudChat::Reset( void )
{
}

int CHudChat::GetChatInputOffset( void )
{
    if ( m_pChatInput->IsVisible() )
    {
        return m_iFontHeight;
    }
    else
        return 0;
}

ConVar zm_cl_chat_color_dev( "zm_cl_chat_color_dev", "255 255 64" );


Color CHudChat::GetClientColor( int clientIndex )
{
    if ( clientIndex == 0 ) // console msg
    {
        return g_ColorBlue;
    }
    else if ( g_PR )
    {
        // Not my fault.
        if ( g_ZMImportanceSystem.GetPlayerImportance( clientIndex ) == ZMIMPORTANCE_DEV )
        {
            CSplitString split( zm_cl_chat_color_dev.GetString(), " " );

            int clr[3] = { 255, 255, 255 };
            for ( int i = 0; i < ARRAYSIZE( clr ); i++ )
            {
                if ( split.Count() > i )
                    clr[i] = Q_atoi( split[i] );
                else
                    break;
            }

            return Color( clr[0], clr[1], clr[2], 255 );
        }


        switch ( g_PR->GetTeam( clientIndex ) )
        {
        case ZMTEAM_HUMAN :     return g_ColorRed;
        case ZMTEAM_ZM :        return g_ColorGreen;
        default	:               return g_ColorGrey;
        }
    }

    return g_ColorYellow;
}
