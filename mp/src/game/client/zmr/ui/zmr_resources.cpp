#include "cbase.h"
#include "hudelement.h"

#include "iclientmode.h"
#include "baseviewport.h"


#include "zmr/c_zmr_player.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr/c_zmr_util.h"


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


    HScheme scheme;
    HFont m_hMediumFont;
    HFont m_hLargeFont;


    int m_nResCount;
    int m_nResCountDif;
    int m_nPopCount;
    int m_nPopMax;
    int m_nSelected;

    CHudTexture* m_pIcons[NUM_ICONS];
};

DECLARE_HUDELEMENT( CZMResourceHud );


CZMResourceHud::CZMResourceHud( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMResourceHud" )
{
    scheme = vgui::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" );

    m_hMediumFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet20", false );
    m_hLargeFont = vgui::scheme()->GetIScheme( scheme )->GetFont( "Trebuchet30", false );
    

    SetPaintBackgroundEnabled( false );
    SetProportional( false );
    SetSize( 120, 100 );


    LoadIcons();
    Reset();
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
    m_nResCountDif = 0;
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
    SetPos( 15, ScreenHeight() - GetTall() );
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

    if ( (newres - m_nResCount) > 0 )
        m_nResCountDif = newres - m_nResCount; 

    m_nResCount = newres;
}

void CZMResourceHud::Paint()
{
    wchar_t text[32];

    
    _snwprintf( text, ARRAYSIZE(text) - 1, L"%i", m_nResCount );

	surface()->DrawSetTextFont( m_hLargeFont );
	surface()->DrawSetTextPos( 45, 5 );
	surface()->DrawSetTextColor( COLOR_WHITE );
	surface()->DrawPrintText( text, wcslen( text ) );



    _snwprintf( text, ARRAYSIZE(text) - 1, L"+ %i", m_nResCountDif );

	surface()->DrawSetTextFont( m_hMediumFont );
	surface()->DrawSetTextPos( 70, 30 );
	surface()->DrawSetTextColor( COLOR_WHITE );
	surface()->DrawPrintText( text, wcslen( text ) );


    _snwprintf( text, ARRAYSIZE(text) - 1, L"%i / %i", m_nPopCount, m_nPopMax );

    if ( m_nSelected > 0 )
        _snwprintf( text, ARRAYSIZE(text) - 1, L"%s (%i)", text, m_nSelected );



	surface()->DrawSetTextFont( m_hMediumFont );
	surface()->DrawSetTextPos( 35, 58 );
	surface()->DrawSetTextColor( COLOR_WHITE );
	surface()->DrawPrintText( text, wcslen( text ) );
    


    int x = 2;
    int y = 5;
    for ( int i = 0; i < NUM_ICONS; i++ )
    {
        if ( m_pIcons[i] )
        {
            //when we get better/larger icons than the current 32x32 ones, replace Width/Height calls with hardcoded 32 size
            const int w = (int)m_pIcons[i]->Width();
            const int h = (int)m_pIcons[i]->Height();

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

            m_pIcons[i]->DrawSelf( x + indent, y, w, h, COLOR_WHITE );

            //for now just assume we move down
            y += h + 25;
        }
    }
}
