#include "cbase.h"
#include <vgui/ISurface.h>


#include "zmr/npcs/c_zmr_zombiebase.h"
#include "zmr_buildmenu_spawnicon.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



using namespace vgui;

//
DECLARE_BUILD_FACTORY( CZMImageRowItemSpawn );

CZMImageRowItemSpawn::CZMImageRowItemSpawn( vgui::Panel* pParent, const char* name ) : CZMImageRowItem( pParent, name )
{
    ResetMe();
}

CZMImageRowItemSpawn::~CZMImageRowItemSpawn()
{
}

void CZMImageRowItemSpawn::ResetMe()
{
    m_flStartTime = 0.0f;
    m_nCount = 0;
    m_iZombieClass = ZMCLASS_INVALID;
    m_bIsPrimary = false;
    m_flDelay = 0.1f;

    m_FillColor = Color( 0, 0, 0, 0 );
}

void CZMImageRowItemSpawn::ApplySettings( KeyValues* kv )
{
    m_bIsPrimary = kv->GetBool( "isprimary", false );

    m_FillColor = kv->GetColor( "fillcolor" );

    BaseClass::ApplySettings( kv );
}

void CZMImageRowItemSpawn::Paint()
{
    auto* pImage = GetPlaceImage();
    if ( !pImage )
        return;


    int x, y;
    int w, h;
    pImage->GetPos( x, y );
    pImage->GetSize( w, h );


    // Draw outline
    if ( !IsOldMenu() )
    {
        const Color clr = Color( 128, 0, 0, 255 );
        surface()->DrawSetColor( clr );
        surface()->DrawOutlinedRect( x, y, x + w, y + h );
    }


    // Draw "time" rect
    if ( m_bIsPrimary )
    {
        UpdateSpawnTime();


        const Color clr = Color( 255, 0, 0, 32 );
        surface()->DrawSetColor( clr );


        float frac = (gpGlobals->curtime - m_flStartTime) / m_flDelay;
        frac = MIN( frac, 1.0f );

        if ( frac > 0.0f )
        {
            frac = MIN( frac, 1.0f );

            surface()->DrawFilledRect( x, y + (1.0f-frac) * h, x + w, y + h );
        }
    }

    BaseClass::Paint();
}

void CZMImageRowItemSpawn::PaintBackground()
{
    auto* pImage = GetPlaceImage();
    if ( !pImage )
        return;


    // Draw background
    if ( m_FillColor[3] != 0 )
    {
        int x, y;
        int w, h;
        pImage->GetPos( x, y );
        pImage->GetSize( w, h );


        surface()->DrawSetColor( m_FillColor );
        surface()->DrawFilledRect( x, y, x + w, y + h );
    }
}

void CZMImageRowItemSpawn::UpdateSpawnTime()
{
    bool bSpawn = CanSpawn();

    if ( bSpawn && m_flStartTime == 0.0f )
    {
        m_flStartTime = gpGlobals->curtime;
        m_flDelay = C_ZMBaseZombie::GetSpawnDelay( (ZombieClass_t)m_iZombieClass );
        m_flDelay = MAX( 0.05f, m_flDelay );
    }
    else if ( !bSpawn )
    {
        m_flStartTime = 0.0f;
    }
}

void CZMImageRowItemSpawn::UpdateData( int count, ZombieClass_t zclass )
{
    if ( !m_nCount || m_nCount > count )
        m_flStartTime = 0.0f;

    m_nCount = count;
    m_iZombieClass = zclass;
}

bool CZMImageRowItemSpawn::CanSpawn() const
{
    auto* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer || !pPlayer->IsZM() )
        return false;

    if ( !pPlayer->HasEnoughResToSpawn( m_iZombieClass ) )
        return false;
    
    if ( !C_ZMBaseZombie::HasEnoughPopToSpawn( m_iZombieClass ) )
        return false;
    

    return true;
}
//
