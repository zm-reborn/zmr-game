#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"


#include "c_zmr_util.h"
#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar zm_cl_showtargetid( "zm_cl_showtargetid", "1", FCVAR_ARCHIVE );



class CZMHudTargetID : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudTargetID, vgui::Panel );


    CZMHudTargetID( const char* name );
    ~CZMHudTargetID();

    virtual void VidInit() OVERRIDE;
    virtual void ApplySchemeSettings( vgui::IScheme* scheme ) OVERRIDE;
    virtual void Paint() OVERRIDE;
    

    bool ShouldRebuildTargetId( C_BasePlayer* pPlayer );
    void BuildTargetIdString( C_BasePlayer* pPlayer );
    void PaintTargetId( C_BasePlayer* pPlayer );

private:
    int         m_iLastEntIndex;
    int         m_nLastHealth;
    float       m_flLastChangeTime;

    wchar_t     m_wszTargetIdStr[128];


    CPanelAnimationVar( vgui::HFont, m_hFont, "TargetIDFont", "TargetID" );
};

DECLARE_HUDELEMENT( CZMHudTargetID );

using namespace vgui;


CZMHudTargetID::CZMHudTargetID( const char* name ) :
    CHudElement( name ), BaseClass( g_pClientMode->GetViewport(), "ZMHudTargetID" )
{
    SetPaintBackgroundEnabled( false );
    SetHiddenBits( HIDEHUD_MISCSTATUS );


    m_flLastChangeTime = 0;
    m_iLastEntIndex = 0;
    m_nLastHealth = 0;
}

CZMHudTargetID::~CZMHudTargetID()
{
}

void CZMHudTargetID::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );


    SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

void CZMHudTargetID::VidInit()
{
    CHudElement::VidInit();

    m_flLastChangeTime = 0;
    m_iLastEntIndex = 0;
    m_nLastHealth = 0;
}

void CZMHudTargetID::Paint()
{
    if ( !zm_cl_showtargetid.GetBool() ) return;


    C_ZMPlayer* pLocalPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pLocalPlayer ) return;

    int iIgnore = 0;

    if ( !pLocalPlayer->IsHuman() )
    {
        // Allow us to see people's target id even if we're dead.
        if ( !pLocalPlayer->IsObserver() )
        {
            return;
        }

        // Ignore our chase target.
        int mode = pLocalPlayer->GetObserverMode();

        if ((mode == OBS_MODE_IN_EYE || mode == OBS_MODE_CHASE)
        &&  pLocalPlayer->GetObserverTarget())
        {
            iIgnore = pLocalPlayer->GetObserverTarget()->entindex();
        }
    }


    // Get our target's ent index
    int iEntIndex = pLocalPlayer->GetIDTarget();
    // Didn't find one?
    if ( !iEntIndex )
    {
        // Check to see if we should clear our ID
        if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.15f)) )
        {
            m_flLastChangeTime = 0;
            m_iLastEntIndex = 0;
        }
        else
        {
            // Keep re-using the old one
            iEntIndex = m_iLastEntIndex;
        }
    }
    else
    {
        m_flLastChangeTime = gpGlobals->curtime;
    }


    if ( !iEntIndex ) return;

    if ( iEntIndex == iIgnore ) return;



    C_BasePlayer* pPlayer = ToBasePlayer( cl_entitylist->GetEnt( iEntIndex ) );

    if ( ShouldRebuildTargetId( pPlayer ) )
    {
        BuildTargetIdString( pPlayer );
    }

    PaintTargetId( pPlayer );


    m_iLastEntIndex = iEntIndex;
}

bool CZMHudTargetID::ShouldRebuildTargetId( C_BasePlayer* pPlayer )
{
    if ( !pPlayer )
        return true;

    if ( pPlayer->entindex() != m_iLastEntIndex )
        return true;

    if ( pPlayer->GetHealth() != m_nLastHealth )
        return true;


    return false;
}

void CZMHudTargetID::BuildTargetIdString( C_BasePlayer* pPlayer )
{
    if ( !pPlayer )
    {
        m_wszTargetIdStr[0] = NULL;
        return;
    }
    


    wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
    wszPlayerName[0] = NULL;



    const char* formHealth = nullptr;
    const char* formatString = "#ZMTargetId_Format";

    

    g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof( wszPlayerName ) );
    

    int hp = pPlayer->GetHealth();

    if ( hp > 80 )
    {
        formHealth = "#ZMTargetId_Healthy";
    }
    else if ( hp > 60 )
    {
        formHealth = "#ZMTargetId_Hurt";
    }
    else if ( hp > 30 )
    {
        formHealth = "#ZMTargetId_Wounded";
    }
    else if ( hp > 10 )
    {
        formHealth = "#ZMTargetId_NearDeath";
    }
    else
    {
        formHealth = "#ZMTargetId_Dead";
    }

    g_pVGuiLocalize->ConstructString(
        m_wszTargetIdStr, sizeof( m_wszTargetIdStr ),
        g_pVGuiLocalize->Find( formatString ), 2,
        wszPlayerName,
        g_pVGuiLocalize->Find( formHealth ) );
}

void CZMHudTargetID::PaintTargetId( C_BasePlayer* pPlayer )
{
    if ( !pPlayer )
        return;

    if ( m_wszTargetIdStr[0] == NULL )
        return;


    Vector screen;
    int xpos, ypos;
    ZMClientUtil::WorldToScreen( pPlayer->GetAbsOrigin(), screen, xpos, ypos );

    int tw, th;
    surface()->GetTextSize( m_hFont, m_wszTargetIdStr, tw, th );


    // Center
    xpos -= tw / 2;


    // Clamp to screen
    if ( ypos < 0 )
    {
        ypos = 0;
    }
    else if ( (ypos + th) > ScreenHeight() )
    {
        ypos = ScreenHeight() - th;
    }
    

    surface()->DrawSetTextFont( m_hFont );
    surface()->DrawSetTextPos( xpos, ypos );
    surface()->DrawSetTextColor( COLOR_WHITE );
    surface()->DrawPrintText( m_wszTargetIdStr, wcslen( m_wszTargetIdStr ) );
}
