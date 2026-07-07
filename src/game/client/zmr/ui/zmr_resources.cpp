#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/ImagePanel.h>

#include "iclientmode.h"
#include "baseviewport.h"


#include "c_zmr_player.h"
#include "zmr_gamerules.h"
#include "zmr_resource_system.h"
#include "npcs/c_zmr_zombiebase.h"
#include "c_zmr_util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


enum// Icon
{ 
	ICON_RES,
	ICON_ZEDS,

	NUM_ICONS,
};

class CZMResourceHud : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMResourceHud, Panel );

    CZMResourceHud( const char* );

    virtual void PerformLayout() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void Init() OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void PaintBackground() OVERRIDE;

private:
    int m_nTexBgId;

    int m_nResCount;
    float m_flResPerMin;
    int m_nPopCount;
    int m_nPopMax;
    int m_nSelected;

    CHudTexture* m_pIcons[NUM_ICONS];

    ImagePanel* m_pResourcesImage;
    ImagePanel* m_pPopulationImage;
    Label* m_pResourceLabel;
    Label* m_pResourceGainRateLabel;
    Label* m_pPopulationLabel;
};

DECLARE_HUDELEMENT( CZMResourceHud );


CZMResourceHud::CZMResourceHud( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudResource" )
{
    SetPaintBackgroundEnabled( true );
    SetProportional( true );
    HScheme scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" );

    
    HFont mediumFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet12", true );
    HFont largeFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet16", true );


    m_pResourcesImage = new ImagePanel( this, "ResourcesImage" );
    m_pResourcesImage->SetImage( vgui::scheme()->GetImage( "miniskull", false ) );
    m_pResourcesImage->SetShouldScaleImage( true );
    m_pPopulationImage = new ImagePanel( this, "PopulationImage" );
    m_pPopulationImage->SetImage( vgui::scheme()->GetImage( "minifigures", false ) );
    m_pPopulationImage->SetShouldScaleImage( true );
    m_pResourceLabel = new Label( this, "ResourceLabel", "00000" );
    m_pResourceLabel->SetFont( largeFont );
    m_pResourceLabel->SizeToContents();
    m_pResourceGainRateLabel = new Label( this, "ResourceGainRateLabel", "0000 rpm" );
    m_pResourceGainRateLabel->SetFont( mediumFont );
    m_pResourceGainRateLabel->SetContentAlignment( Label::Alignment::a_northeast );
    m_pResourceGainRateLabel->SizeToContents();
    m_pPopulationLabel = new Label( this, "PopulationLabel", "000 / 000 (000)" );
    m_pPopulationLabel->SetFont( mediumFont );
    m_pPopulationLabel->SizeToContents();
    
    Reset();


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hud_bg_zmres", true, false );
}

void CZMResourceHud::Init()
{
    Reset();
}

void CZMResourceHud::Reset()
{
    m_nResCount = 0;
    m_flResPerMin = 0.0f;
    m_nPopCount = 0;
    m_nPopMax = 0;
    m_nSelected = 0;
}

void CZMResourceHud::VidInit()
{
    Reset();
}

void CZMResourceHud::PerformLayout()
{
    int width = QuickPropScale( 75 );
    int height = QuickPropScale( 70 );

    SetBounds( 0, GetParent()->GetTall() - height, width, height );

    int image_size = QuickPropScale( 16 );
    int image_size_half = QuickPropScale( 8 );

    m_pResourcesImage->SetPos( QuickPropScale( 6 ), QuickPropScale( 16 ) );
    m_pResourcesImage->SetSize( image_size, image_size );
    m_pResourceLabel->SetPos( QuickPropScale( 24 ), QuickPropScale( 16 ) );
    m_pResourceLabel->SizeToContents();

    m_pResourceGainRateLabel->SetPos( QuickPropScale( 18 ), QuickPropScale( 30 ) );
    m_pResourceGainRateLabel->SizeToContents();

    m_pPopulationImage->SetPos( QuickPropScale( 10 ), QuickPropScale( 52 ) + 1 );
    m_pPopulationImage->SetSize( image_size_half, image_size_half );
    m_pPopulationLabel->SetPos( QuickPropScale( 22 ), QuickPropScale( 50 ) );
    m_pPopulationLabel->SizeToContents();
}

void CZMResourceHud::OnThink()
{
    C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );
    if ( !pPlayer ) return;


    SetVisible( pPlayer->IsZM() );

    if ( !IsVisible() ) return;

    static wchar_t text[32];
    text[0] = L'\0';

    C_ZMRules* pRules = ZMRules();

    int selectedZombieCount = ZMClientUtil::GetSelectedZombieCount();
    int newPopCount = pRules ? pRules->GetZombiePop() : 0;
    int zombieMax = zm_sv_zombiemax.GetInt();
    int newResources = pPlayer->GetResources();

    g_ZMResourceSystem.UpdateState();

    float newResPerMin = g_ZMResourceSystem.GetResourcesPerMinute();
    


    if ( m_nResCount != newResources )
    {
        V_snwprintf( text, ARRAYSIZE( text ), L"%i", newResources );
        m_pResourceLabel->SetText( text );
        m_pResourceLabel->SizeToContents();
    }

    if ( m_flResPerMin != newResPerMin )
    {
        V_snwprintf( text, ARRAYSIZE( text ), L"%.0f rpm", newResPerMin );
        m_pResourceGainRateLabel->SetText( text );
        m_pResourceGainRateLabel->SizeToContents();
    }

    if ( m_nSelected != selectedZombieCount || m_nPopCount != newPopCount || m_nPopMax != zombieMax )
    {
        V_snwprintf( text, ARRAYSIZE( text ), L"%i / %i", newPopCount, zombieMax );
        if ( selectedZombieCount > 0 )
        {
            V_snwprintf( text, ARRAYSIZE( text ), L"%s (%i)", text, selectedZombieCount );
        }
        m_pPopulationLabel->SetText( text );
        m_pPopulationLabel->SizeToContents();
    }

    m_nSelected = selectedZombieCount;
    m_nPopCount = newPopCount;
    m_nPopMax = zombieMax;
    m_nResCount = newResources;
    m_flResPerMin = newResPerMin;
}

void CZMResourceHud::PaintBackground()
{
    int sizex = GetWide();
    int sizey = GetTall();

    vgui::surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( 0, 0, sizex, sizey );
}
