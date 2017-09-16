#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ihudlcd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


#define SIDE_LEFT 0
#define SIDE_RIGHT 1

//-----------------------------------------------------------------------------
// Purpose: Displays current ammunition level
//-----------------------------------------------------------------------------
class CZMHudAmmo : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE( CZMHudAmmo, Panel );

public:
    CZMHudAmmo( const char *pElementName );
    void Init( void );
    void VidInit( void );
    void Reset();

    void SetAmmo( C_BaseCombatWeapon*, int ammo, int ammo2 );
    virtual void Paint( void );

protected:
    virtual void OnThink();

    void UpdatePlayerAmmo();
    

    void PaintGlowString( const Color& clr, float blur, int xpos, int ypos, const wchar_t* str, int side );
    void PaintString( const HFont& font, const Color& clr, int xpos, int ypos, const wchar_t* str );

private:
    CHandle<C_BaseCombatWeapon> m_hCurrentActiveWeapon;
    int m_iAmmo;
    int m_iAmmo2;
    //CHudTexture* m_iconPrimaryAmmo;


    CPanelAnimationVar( float, m_flClipBlur, "ClipBlur", "0" );
    CPanelAnimationVar( float, m_flResBlur, "ResBlur", "0" );

    CPanelAnimationVar( Color, m_ClipColor, "ClipColor", "ZMFgColor" );
    CPanelAnimationVar( Color, m_MidColor, "MidColor", "ZMFgColor" );
    CPanelAnimationVar( Color, m_ResColor, "ResColor", "ZMFgColor" );
    
    CPanelAnimationVar( HFont, m_hFont, "AmmoBarFont", "HudNumbers" );
    CPanelAnimationVar( HFont, m_hGlowFont, "AmmoBarGlowFont", "HudNumbersGlow" );
    CPanelAnimationVar( HFont, m_hShadowFont, "AmmoBarShadowFont", "HudNumbersShadow" );

    CPanelAnimationVarAliasType( float, m_ClipX, "ClipX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ClipY, "ClipY", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_MidX, "MidX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_MidY, "MidY", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ResX, "ResX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ResY, "ResY", "0", "proportional_float" );

    CPanelAnimationVarAliasType( float, m_MidWidth, "MidWidth", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_MidHeight, "MidHeight", "0", "proportional_float" );

    bool m_bDisplaySecondary;
};

DECLARE_HUDELEMENT( CZMHudAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CZMHudAmmo::CZMHudAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudAmmo" )
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );
}

void CZMHudAmmo::Init( void )
{
    m_iAmmo		= -1;
    m_iAmmo2	= -1;
    
    //m_iconPrimaryAmmo = NULL;


    SetPaintBackgroundEnabled( false );
}

void CZMHudAmmo::VidInit( void )
{
}

void CZMHudAmmo::Reset()
{
    //BaseClass::Reset();

    m_hCurrentActiveWeapon = NULL;
    m_iAmmo = 0;
    m_iAmmo2 = 0;

    UpdatePlayerAmmo();
}

void CZMHudAmmo::UpdatePlayerAmmo()
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    C_BaseCombatWeapon* pWeapon = GetActiveWeapon();

    
    if ( !pWeapon || !pWeapon->UsesPrimaryAmmo() || pWeapon->GetMaxClip1() <= 1 )
    {
        SetPaintEnabled( false );
        return;
    }

    SetPaintEnabled( true );
    
    // get the ammo in our clip
    int ammo1 = pWeapon->Clip1();
    int ammo2;
    if ( ammo1 < 0 )
    {
        // we don't use clip ammo, just use the total ammo count
        ammo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
        ammo2 = 0;
    }
    else
    {
        // we use clip ammo, so the second ammo is the total ammo
        ammo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
    }

    //hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", ammo1 ) );
    //hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", ammo2 ) );

    SetAmmo( pWeapon, ammo1, ammo2 );

    if ( pWeapon != m_hCurrentActiveWeapon )
    {
        // update whether or not we show the total ammo display
        if ( pWeapon->UsesClipsForAmmo1() )
        {
            m_bDisplaySecondary = true;
            //g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "WeaponUsesClips" );
        }
        else
        {
            //g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "WeaponDoesNotUseClips" );
            m_bDisplaySecondary = false;
        }

        //g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "WeaponChanged" );
        m_hCurrentActiveWeapon = pWeapon;
    }
}

void CZMHudAmmo::OnThink()
{
    UpdatePlayerAmmo();
}

void CZMHudAmmo::SetAmmo( C_BaseCombatWeapon* pWeapon, int ammo, int ammo2 )
{
    if ( ammo != m_iAmmo )
    {
        int criticalclip = 0;

        if ( pWeapon && pWeapon->GetMaxClip1() > 5 )
        {
            criticalclip = pWeapon->GetMaxClip1() * 0.25f;
        }


        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMClipChanged" );

        if ( ammo <= criticalclip )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMClipCritical" );
        }
        else
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMClipOk" );
        }

        m_iAmmo = ammo;
    }

    if ( ammo2 != m_iAmmo2 )
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMResChanged" );

        if ( ammo2 == 0 )
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMResCritical" );
        }
        else
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMResOk" );
        }

        m_iAmmo2 = ammo2;
    }
}

void CZMHudAmmo::PaintString( const HFont& font, const Color& clr, int xpos, int ypos, const wchar_t* str )
{
	surface()->DrawSetTextFont( font );
    surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextPos( xpos, ypos );
	surface()->DrawUnicodeString( str );
}

void CZMHudAmmo::PaintGlowString( const Color& clr, float blur, int xpos, int ypos, const wchar_t* str, int side )
{
    if ( side != SIDE_LEFT )
    {
        int w, h;
        surface()->GetTextSize( m_hFont, str, w, h );

        if ( side == SIDE_RIGHT )
            xpos -= w;
    }


    PaintString( m_hShadowFont, Color( 0, 0, 0, 255 ), xpos, ypos, str );


	for ( float fl = min( 1.0f, blur ); fl > 0.0f; fl -= 0.1f )
	{
        Color col = clr;
        col[3] = 150 * fl;

		PaintString( m_hGlowFont, col, xpos, ypos, str );
	}

    PaintString( m_hFont, clr, xpos, ypos, str );
}

void CZMHudAmmo::Paint()
{
    wchar_t szAmmo[8];
    V_snwprintf( szAmmo, ARRAYSIZE( szAmmo ), L"%i", m_iAmmo );

    PaintGlowString( m_ClipColor, m_flClipBlur, m_ClipX, m_ClipY, szAmmo, SIDE_RIGHT );


    if ( m_bDisplaySecondary )
    {
        surface()->DrawSetColor( m_MidColor );
        surface()->DrawFilledRect( m_MidX, m_MidY, m_MidX + m_MidWidth, m_MidY + m_MidHeight );

        V_snwprintf( szAmmo, ARRAYSIZE( szAmmo ), L"%i", m_iAmmo2 );

        PaintGlowString( m_ResColor, m_flResBlur, m_ResX, m_ResY, szAmmo, SIDE_LEFT );
    }
}
