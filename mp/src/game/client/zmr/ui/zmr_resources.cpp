#include "cbase.h"
#include "hudelement.h"

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
    ~CZMResourceHud();

    void VidInit() OVERRIDE;
    void Init() OVERRIDE;
    void Reset() OVERRIDE;
    void OnThink() OVERRIDE;
    void Paint() OVERRIDE;

private:
    void LoadIcons();
    void Reposition();

    void PaintBg();


    HFont m_hMediumFont;
    HFont m_hLargeFont;

    int m_nTexBgId;

    int m_nResCount;
    float m_flResPerMin;
    int m_nPopCount;
    int m_nPopMax;
    int m_nSelected;

    CHudTexture* m_pIcons[NUM_ICONS];


    CPanelAnimationVar( Color, m_BgColor, "BgColor", "ZMHudBgColor" );
    CPanelAnimationVar( Color, m_FgColor, "FgColor", "ZMFgColor" );
};

DECLARE_HUDELEMENT( CZMResourceHud );


CZMResourceHud::CZMResourceHud( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudResource" )
{
    HScheme scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" );

    m_hMediumFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet20", false );
    m_hLargeFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet30", false );
    

    SetPaintBackgroundEnabled( false );
    SetProportional( false );
    SetSize( 140, 140 );


    LoadIcons();
    Reset();


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hud_bg_zmres", true, false );
}

CZMResourceHud::~CZMResourceHud()
{
    // HUD icons are automatically loaded/unloaded. gHUD.GetIcon doesn't do any allocation.
    /*for ( int i = 0; i < NUM_ICONS; i++ )
    {
        if ( m_pIcons[i] )
        {
            delete m_pIcons[i];
            m_pIcons[i] = NULL;
        }
    }*/
}

void CZMResourceHud::Init()
{
    Reset();
}

void CZMResourceHud::LoadIcons()
{
	m_pIcons[ICON_RES] = gHUD.GetIcon( "icon_resources" );
	m_pIcons[ICON_ZEDS] = gHUD.GetIcon( "icon_figures" );
}

void CZMResourceHud::Reset()
{
    m_nResCount = 0;
    m_flResPerMin = 0.0f;
    m_nPopCount = 0;
    m_nSelected = 0;

    Reposition();
}

void CZMResourceHud::VidInit()
{
    LoadIcons();
    Reset();
}

void CZMResourceHud::Reposition()
{
    SetPos( 0, ScreenHeight() - GetTall() );
}

void CZMResourceHud::OnThink()
{
    C_ZMPlayer* pPlayer = ToZMPlayer( C_BasePlayer::GetLocalPlayer() );
    if ( !pPlayer ) return;


    SetVisible( pPlayer->IsZM() );

    if ( !IsVisible() ) return;



    C_ZMRules* pRules = ZMRules();

    m_nSelected = ZMClientUtil::GetSelectedZombieCount();
    m_nPopCount = pRules ? pRules->GetZombiePop() : 0;
    m_nPopMax = zm_sv_zombiemax.GetInt();

    int newres = pPlayer->GetResources();

    g_ZMResourceSystem.UpdateState();
    m_flResPerMin = g_ZMResourceSystem.GetResourcesPerMinute();

    m_nResCount = newres;
}

void CZMResourceHud::PaintBg()
{
    int sizex = GetWide();
    int sizey = GetTall();

    vgui::surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( 0, 0, sizex, sizey );
}

void CZMResourceHud::Paint()
{
    PaintBg();


    static wchar_t text[32];
    int w, h;

    const int offsety = 45;
    
    _snwprintf( text, ARRAYSIZE(text) - 1, L"%i", m_nResCount );

	surface()->DrawSetTextFont( m_hLargeFont );
	surface()->DrawSetTextPos( 60, offsety + 0 );
	surface()->DrawSetTextColor( m_FgColor );
	surface()->DrawPrintText( text, wcslen( text ) );

    surface()->GetTextSize( m_hLargeFont, text, w, h );


    _snwprintf( text, ARRAYSIZE(text) - 1, L"%.0f rpm", m_flResPerMin );

    int w2;
    surface()->GetTextSize( m_hMediumFont, text, w2, h );

	surface()->DrawSetTextFont( m_hMediumFont );
	surface()->DrawSetTextPos( 112 - w2, offsety + 27 );
	surface()->DrawSetTextColor( m_FgColor );
	surface()->DrawPrintText( text, wcslen( text ) );


    _snwprintf( text, ARRAYSIZE(text) - 1, L"%i / %i", m_nPopCount, m_nPopMax );

    if ( m_nSelected > 0 )
        _snwprintf( text, ARRAYSIZE(text) - 1, L"%s (%i)", text, m_nSelected );



	surface()->DrawSetTextFont( m_hMediumFont );
	surface()->DrawSetTextPos( 50, offsety + 53 );
	surface()->DrawSetTextColor( m_FgColor );
	surface()->DrawPrintText( text, wcslen( text ) );
    


    int x = 17;
    int y = offsety + 0;
    for ( int i = 0; i < NUM_ICONS; i++ )
    {
        if ( m_pIcons[i] )
        {
            //when we get better/larger icons than the current 32x32 ones, replace Width/Height calls with hardcoded 32 size
            w = (int)m_pIcons[i]->Width();
            h = (int)m_pIcons[i]->Height();

            //can't avoid some icon-specific positioning
            int indent = 2;
            switch ( i )
            {
                case ICON_ZEDS:
                    indent = 10;
                    break;
                default:
                    break;
            }

            m_pIcons[i]->DrawSelf( x + indent, y, w, h, m_FgColor );

            //for now just assume we move down
            y += h + 25;
        }
    }
}
