#include "cbase.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "ihudlcd.h"
#include <engine/IEngineSound.h>

#include "zmr_shareddefs.h"
#include "c_zmr_importancesystem.h"

#include "zmr_hud_chat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_chat_color_dev( "zm_cl_chat_color_dev", "255 255 64" );
ConVar zm_cl_chat_color_lead( "zm_cl_chat_color_lead", "231 76 60" );


void UTIL_ParseColorFromString( const char* str, int clr[], int nColors );


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

Color CHudChat::GetClientColor( int clientIndex )
{
    if ( clientIndex == 0 ) // console msg
    {
        return g_ColorBlue;
    }
    else if ( g_PR )
    {
        switch ( g_PR->GetTeam( clientIndex ) )
        {
        case ZMTEAM_HUMAN :     return g_ColorRed;
        case ZMTEAM_ZM :        return g_ColorGreen;
        default	:               return g_ColorGrey;
        }
    }

    return g_ColorYellow;
}

//
// Override SayText2 for importance changes.
//
void CHudChat::MsgFunc_SayText2( bf_read &msg )
{
    // Got message during connection
    if ( !g_PR )
        return;

    int client = msg.ReadByte();
    bool bWantsToChat = msg.ReadByte();

    wchar_t szBuf[6][256];
    char untranslated_msg_text[256];
    wchar_t *msg_text = ReadLocalizedString( msg, szBuf[0], sizeof( szBuf[0] ), false, untranslated_msg_text, sizeof( untranslated_msg_text ) );


    // keep reading strings and using C format strings for subsituting the strings into the localised text string
    ReadChatTextString ( msg, szBuf[1], sizeof( szBuf[1] ) );		// player name
    ReadChatTextString ( msg, szBuf[2], sizeof( szBuf[2] ) );		// chat text
    ReadLocalizedString( msg, szBuf[3], sizeof( szBuf[3] ), true );
    ReadLocalizedString( msg, szBuf[4], sizeof( szBuf[4] ), true );


    // Add star next to dev name.
    auto importance = g_ZMImportanceSystem.GetPlayerImportance( client );
    if ( importance == ZMIMPORTANCE_DEV || importance == ZMIMPORTANCE_LEAD )
    {
        //
        // %s is for char, %ls is for wide-char
        //

        // Rewrite the name to include the text color.
        wchar_t szTemp[256];
        V_wcsncpy( szTemp, szBuf[1], sizeof( szTemp ) );
        V_snwprintf( szBuf[1], ARRAYSIZE( szBuf[1] ), L"%ls\x01", szTemp );


        // Rewrite the format to include the star and the coloring.
        V_wcsncpy( szTemp, szBuf[0], sizeof( szTemp ) );

        // Get rid of the x02 at the start.
        int index = 0;
        if ( szTemp[0] == '\x02' )
        {
            index = 1;
        }

        auto clrstr = importance == ZMIMPORTANCE_LEAD ? zm_cl_chat_color_lead.GetString() : zm_cl_chat_color_dev.GetString();
        int clr[3];
        UTIL_ParseColorFromString( clrstr, clr, ARRAYSIZE( clr ) );

        // x03 = team color
        // \a = hex color
        V_snwprintf( szBuf[0], ARRAYSIZE( szBuf[0] ), L"\x01\a%x%x%x★ \x03%ls", clr[0], clr[1], clr[2], &szTemp[index] );
    }


    g_pVGuiLocalize->ConstructString( szBuf[5], sizeof( szBuf[5] ), msg_text, 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

    char ansiString[512];
    g_pVGuiLocalize->ConvertUnicodeToANSI( ConvertCRtoNL( szBuf[5] ), ansiString, sizeof( ansiString ) );

    if ( bWantsToChat )
    {
        int iFilter = CHAT_FILTER_NONE;

        if ( client > 0 && (g_PR->GetTeam( client ) != g_PR->GetTeam( GetLocalPlayerIndex() )) )
        {
            iFilter = CHAT_FILTER_PUBLICCHAT;
        }

        // print raw chat text
        ChatPrintf( client, iFilter, "%s", ansiString );

        Msg( "%s\n", RemoveColorMarkup(ansiString) );

        CLocalPlayerFilter filter;
        C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );
    }
    else
    {
        // print raw chat text
        ChatPrintf( client, GetFilterForString( untranslated_msg_text), "%s", ansiString );
    }
}
