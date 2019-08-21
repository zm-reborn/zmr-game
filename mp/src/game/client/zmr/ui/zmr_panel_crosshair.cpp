#include "cbase.h"


#include "zmr_panel_crosshair.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_BUILD_FACTORY( CZMPanelCrosshair );

CZMPanelCrosshair::CZMPanelCrosshair( vgui::Panel* parent, const char* name ) : vgui::EditablePanel( parent, name )
{
    m_pDrawCrosshair = nullptr;
}

CZMPanelCrosshair::~CZMPanelCrosshair()
{
    //delete m_pAccuracyCrosshair;
    //m_pDrawCrosshair = nullptr;
}

void CZMPanelCrosshair::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    SetBgColor( GetSchemeColor( "CZMPanelCrosshair.BgColor", Color( 0, 0, 0, 196 ), pScheme ) );
}

void CZMPanelCrosshair::SetCrosshairToDraw( CZMBaseCrosshair* pCross )
{
    m_pDrawCrosshair = pCross;
}

void CZMPanelCrosshair::Paint()
{
    if ( !m_pDrawCrosshair )
    {
        return;
    }


    CZMBaseDynamicCrosshair* pDynamic = dynamic_cast<CZMBaseDynamicCrosshair*>( m_pDrawCrosshair );
    if ( pDynamic )
    {
        // Simulate what the dynamic movement will look like.
        pDynamic->SetOverrideDynamicScale(
            clamp(
                cos( gpGlobals->curtime ),
                0.0f,
                1.0f ) );
    }


    int cx, cy;
    GetSize( cx, cy );
    cx /= 2;
    cy /= 2;

    // Make sure to draw the crosshair in the center of our panel element.
    m_pDrawCrosshair->SetOverrideCenter( cx, cy );


    m_pDrawCrosshair->Draw();


    m_pDrawCrosshair->SetOverrideCenter( -1, -1 );

    if ( pDynamic )
    {
        pDynamic->SetOverrideDynamicScale( -1.0f );
    }
}
