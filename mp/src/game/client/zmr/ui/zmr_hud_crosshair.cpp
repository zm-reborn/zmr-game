#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "hudelement.h"
#include "iclientmode.h"

#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>


#include "c_zmr_player.h"
#include "c_zmr_crosshair.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar zm_cl_zmcrosshair;


class CZMHudCrosshair : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CZMHudCrosshair, vgui::Panel );

    CZMHudCrosshair( const char* pElementName );
    ~CZMHudCrosshair();


    virtual void ApplySchemeSettings( vgui::IScheme* scheme ) OVERRIDE;


    virtual bool ShouldDraw() OVERRIDE;
    virtual void Paint() OVERRIDE;
};

DECLARE_HUDELEMENT( CZMHudCrosshair );

using namespace vgui;

CZMHudCrosshair::CZMHudCrosshair( const char* pElementName ) : CHudElement( pElementName ), Panel( g_pClientMode->GetViewport(), "ZMHudCrosshair" )
{
    SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );
}

CZMHudCrosshair::~CZMHudCrosshair()
{
}

void CZMHudCrosshair::ApplySchemeSettings( IScheme* scheme )
{
    BaseClass::ApplySchemeSettings( scheme );

    SetPaintBackgroundEnabled( false );
    SetSize( ScreenWidth(), ScreenHeight() );

    SetForceStereoRenderToFrameBuffer( true );
}

bool CZMHudCrosshair::ShouldDraw()
{
    if ( !CHudElement::ShouldDraw() )
        return false;


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer )
        return false;


    C_BaseCombatWeapon* pWeapon = pPlayer->GetActiveWeapon();
    if ( pWeapon && !pWeapon->ShouldDrawCrosshair() )
        return false;

    if ( !zm_cl_zmcrosshair.GetBool() && ToZMPlayer( pPlayer )->IsZM() )
    {
        return false;
    }


    return  !engine->IsPaused()
        &&  !(pPlayer->GetFlags() & FL_FROZEN)
        &&  g_pClientMode->ShouldDrawCrosshair()
        &&  pPlayer->entindex() == render->GetViewEntity()
        &&  !pPlayer->IsInVGuiInputMode()
        &&  ( pPlayer->IsAlive() ||	(pPlayer->GetObserverTarget() != nullptr && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE) );
}

void CZMHudCrosshair::Paint()
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer )
        return;

    if ( pPlayer->IsZM() )
    {
        // Draw ZM's own unique crosshair.
        if ( zm_cl_zmcrosshair.GetBool() )
        {
            CZMBaseCrosshair* pCross = ZMGetCrosshair( "ZM" );

            if ( pCross )
            {
                pCross->Draw();
            }
        }

        return;
    }

    C_ZMBaseWeapon* pWep;

    if ( !pPlayer->IsAlive() )
    {
        C_ZMPlayer* pObserved = ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) ? ToZMPlayer( pPlayer->GetObserverTarget() ) : nullptr;
        if ( pObserved )
        {
            pWep = static_cast<C_ZMBaseWeapon*>( pObserved->GetActiveWeapon() );
        }
        else
        {
            pWep = nullptr;
        }
    }
    else
    {
        pWep = static_cast<C_ZMBaseWeapon*>( pPlayer->GetActiveWeapon() );
    }

    if ( !pWep )
        return;


    CZMBaseCrosshair* pCross = pWep->GetWeaponCrosshair();
    if ( pCross )
    {
        pCross->Draw();
    }
}
