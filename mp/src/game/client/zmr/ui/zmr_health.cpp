//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CZMHudHealth class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


#define INIT_HEALTH -1

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CZMHudHealth : public CHudElement, public Panel
//class CZMHudHealth : public CHudElement, public CHudNumericDisplay
{
    //DECLARE_CLASS_SIMPLE( CZMHudHealth, CHudNumericDisplay );
    DECLARE_CLASS_SIMPLE( CZMHudHealth, Panel );
    
public:
    CZMHudHealth( const char *pElementName );
    virtual void Init( void );
    virtual void VidInit( void );
    virtual void Reset( void );
    virtual void OnThink();
    virtual void Paint() OVERRIDE;
    void MsgFunc_Damage( bf_read &msg );


    void PaintString( HFont, int, int, const wchar_t* );

private:
    // old variables
    int		m_iHealth;
    
    int		m_bitsDamage;


    CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
    CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
    //CPanelAnimationVar( Color, m_Ammo2Color, "Ammo2Color", "FgColor" );
    
    CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudNumbers" );
    CPanelAnimationVar( vgui::HFont, m_hNumberGlowFont, "NumberGlowFont", "HudNumbersGlow" );
    CPanelAnimationVar( vgui::HFont, m_hSmallNumberFont, "SmallNumberFont", "HudNumbersSmall" );
    //CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};	

DECLARE_HUDELEMENT( CZMHudHealth );
DECLARE_HUD_MESSAGE( CZMHudHealth, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CZMHudHealth::CZMHudHealth( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "HudHealth" )
//CZMHudHealth::CZMHudHealth( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay(NULL, "HudHealth")
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudHealth::Init()
{
    HOOK_HUD_MESSAGE( CZMHudHealth, Damage );
    Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudHealth::Reset()
{
    m_iHealth		= INIT_HEALTH;
    m_bitsDamage	= 0;

    //wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_HEALTH");

    //if (tempString)
    {
        //SetLabelText(tempString);
    }
    //else
    {
        //SetLabelText(L"HEALTH");
    }
    //SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudHealth::VidInit()
{
    Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMHudHealth::OnThink()
{
    int newHealth = 0;
    C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
    if ( local )
    {
        // Never below zero
        newHealth = MAX( local->GetHealth(), 0 );
    }

    // Only update the fade if we've changed health
    if ( newHealth == m_iHealth )
    {
        return;
    }

    m_iHealth = newHealth;

    if ( m_iHealth >= 20 )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthIncreasedAbove20" );
    }
    else if ( m_iHealth > 0 )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthIncreasedBelow20" );
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthLow" );
    }

    //SetDisplayValue(m_iHealth);
}

void CZMHudHealth::PaintString( HFont font, int xpos, int ypos, const wchar_t* str )
{
	surface()->DrawSetTextFont( font );

	surface()->DrawSetTextPos( xpos, ypos );
	surface()->DrawUnicodeString( str );
}

void CZMHudHealth::Paint()
{
	// draw our numbers
	surface()->DrawSetTextColor( GetFgColor() );

    wchar_t hp[16];
    V_snwprintf( hp, ARRAYSIZE( hp ), L"%d", m_iHealth );

	// draw the overbright blur
	for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
	{
		if (fl >= 1.0f)
		{
			PaintString( m_hNumberGlowFont, 0, 0, hp );
		}
		else
		{
			// draw a percentage of the last one
			Color col = GetFgColor();
			col[3] *= fl;
			surface()->DrawSetTextColor(col);
			PaintString( m_hNumberGlowFont, 0, 0, hp );
		}
	}

	//PaintLabel();
}

void CZMHudHealth::MsgFunc_Damage( bf_read &msg )
{
    int armor = msg.ReadByte();	// armor
    int damageTaken = msg.ReadByte();	// health
    long bitsDamage = msg.ReadLong(); // damage bits
    bitsDamage; // variable still sent but not used

    Vector vecFrom;

    vecFrom.x = msg.ReadBitCoord();
    vecFrom.y = msg.ReadBitCoord();
    vecFrom.z = msg.ReadBitCoord();

    // Actually took damage?
    if ( damageTaken > 0 || armor > 0 )
    {
        if ( damageTaken > 0 )
        {
            // start the animation
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthDamageTaken" );
        }
    }
}