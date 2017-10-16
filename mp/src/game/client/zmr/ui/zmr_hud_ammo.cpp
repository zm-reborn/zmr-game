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
    

    void PaintGlowString( const HFont& font, const HFont& glowfont, const HFont& shadowfont, const Color& clr, float blur, int xpos, int ypos, const wchar_t* str, int side );
    void PaintString( const HFont& font, const Color& clr, int xpos, int ypos, const wchar_t* str );
	void PaintBg();

private:
    CHandle<C_BaseCombatWeapon> m_hCurrentActiveWeapon;
    int m_iAmmo;
    int m_iAmmo2;
    //CHudTexture* m_iconPrimaryAmmo;

	int m_nTexPanelBgId;


    CPanelAnimationVar( float, m_flClipBlur, "ClipBlur", "0" );
    CPanelAnimationVar( float, m_flResBlur, "ResBlur", "0" );

    CPanelAnimationVar( Color, m_ClipColor, "ClipColor", "ZMFgColor" );
    CPanelAnimationVar( Color, m_ResColor, "ResColor", "ZMFgColor" );
	CPanelAnimationVar( Color, m_BgColor, "BgColor", "ZMHudBgColor" );
    
    CPanelAnimationVar( HFont, m_hFont, "AmmoBarFont", "HudNumbers" );
    CPanelAnimationVar( HFont, m_hGlowFont, "AmmoBarGlowFont", "HudNumbersGlow" );
    CPanelAnimationVar( HFont, m_hShadowFont, "AmmoBarShadowFont", "HudNumbersShadow" );

    CPanelAnimationVar( HFont, m_hResFont, "AmmoBarResFont", "HudNumbers" );
    CPanelAnimationVar( HFont, m_hResGlowFont, "AmmoBarResGlowFont", "HudNumbersGlow" );
    CPanelAnimationVar( HFont, m_hResShadowFont, "AmmoBarResShadowFont", "HudNumbersShadow" );

    CPanelAnimationVarAliasType( float, m_ClipX, "ClipX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ClipY, "ClipY", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ResX, "ResX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_ResY, "ResY", "0", "proportional_float" );

    CPanelAnimationVarAliasType( float, m_flBgX, "BgX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flBgY, "BgY", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flBgSizeX, "BgSizeX", "0", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flBgSizeY, "BgSizeY", "0", "proportional_float" );

    bool m_bDisplaySecondary;
};

DECLARE_HUDELEMENT( CZMHudAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CZMHudAmmo::CZMHudAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudAmmo" )
{
    SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );

    m_nTexPanelBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexPanelBgId, "zmr_effects/hud_bg_ammo", true, false );
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

void CZMHudAmmo::PaintGlowString( const HFont& font, const HFont& glowfont, const HFont& shadowfont, const Color& clr, float blur, int xpos, int ypos, const wchar_t* str, int side )
{
    if ( side != SIDE_LEFT )
    {
        int w, h;
        surface()->GetTextSize( font, str, w, h );

        if ( side == SIDE_RIGHT )
            xpos -= w;
    }


    PaintString( shadowfont, Color( 0, 0, 0, 255 ), xpos, ypos, str );


	for ( float fl = min( 1.0f, blur ); fl > 0.0f; fl -= 0.1f )
	{
        Color col = clr;
        col[3] = 150 * fl;

		PaintString( glowfont, col, xpos, ypos, str );
	}

    PaintString( font, clr, xpos, ypos, str );
}

void CZMHudAmmo::PaintBg()
{
    int size_x = m_flBgSizeX;
    int size_y = m_flBgSizeY;

    int offset_x = m_flBgX;
    int offset_y = m_flBgY;


    surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( m_nTexPanelBgId );
    surface()->DrawTexturedRect( offset_x, offset_y, offset_x + size_x, offset_y + size_y );
}

void CZMHudAmmo::Paint()
{
	PaintBg();


    wchar_t szAmmo[8];
    V_snwprintf( szAmmo, ARRAYSIZE( szAmmo ), L"%i", m_iAmmo );

    PaintGlowString( m_hFont, m_hGlowFont, m_hShadowFont, m_ClipColor, m_flClipBlur, m_ClipX, m_ClipY, szAmmo, SIDE_RIGHT );


    if ( m_bDisplaySecondary )
    {
        int w, h, hsmall;
        surface()->GetTextSize( m_hFont, szAmmo, w, h );
		surface()->GetTextSize( m_hResFont, szAmmo, w, hsmall );

		int pos_y = m_ClipY + ( h - hsmall );

        V_snwprintf( szAmmo, ARRAYSIZE( szAmmo ), L"%i", m_iAmmo2 );

        PaintGlowString( m_hResFont, m_hResGlowFont, m_hResShadowFont, m_ResColor, m_flResBlur, m_ResX, pos_y, szAmmo, SIDE_LEFT );
    }
}
