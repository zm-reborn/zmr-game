#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_graphics.h"


extern ConVar zm_cl_glow_weapon_enabled;
extern ConVar zm_cl_glow_ammo_enabled;
extern ConVar zm_cl_zmvision_dlight;
extern ConVar zm_cl_silhouette_onlyzmvision;
extern ConVar g_ragdoll_maxcount;


CZMOptionsSubGraphics::CZMOptionsSubGraphics( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubgraphics.res" );

    LoadItem( &m_pCheck_WepGlow, "CheckWeaponGlow" );
    LoadItem( &m_pCheck_AmmoGlow, "CheckAmmoGlow" );
    LoadItem( &m_pCheck_VisionDynLight, "CheckVisionDynLight" );
    LoadItem( &m_pCheck_SilhouetteVision, "CheckSilhouetteVision" );
    LoadItem( &m_pSlider_MaxRagdolls, "MaxRagdollsSlider" );
    LoadItem( &m_pTextEntry_MaxRagdolls, "MaxRagdollsEntry" );


    if ( FailedLoad() ) return;


    m_pSlider_MaxRagdolls->SetRange( 0, 100 );
    m_pSlider_MaxRagdolls->AddActionSignalTarget( this );
}

CZMOptionsSubGraphics::~CZMOptionsSubGraphics()
{
}

void CZMOptionsSubGraphics::OnApplyChanges()
{
    if ( FailedLoad() ) return;

    zm_cl_glow_weapon_enabled.SetValue( m_pCheck_WepGlow->IsSelected() ? 1 : 0 );
    zm_cl_glow_ammo_enabled.SetValue( m_pCheck_AmmoGlow->IsSelected() ? 1 : 0 );
    zm_cl_zmvision_dlight.SetValue( m_pCheck_VisionDynLight->IsSelected() ? 1 : 0 );
    zm_cl_silhouette_onlyzmvision.SetValue( m_pCheck_SilhouetteVision->IsSelected() ? 1 : 0 );
    g_ragdoll_maxcount.SetValue( m_pSlider_MaxRagdolls->GetValue() );
}

void CZMOptionsSubGraphics::OnResetData()
{
    if ( FailedLoad() ) return;


    m_pCheck_WepGlow->SetSelected( zm_cl_glow_weapon_enabled.GetBool() );
    m_pCheck_AmmoGlow->SetSelected( zm_cl_glow_ammo_enabled.GetBool() );
    m_pCheck_VisionDynLight->SetSelected( zm_cl_zmvision_dlight.GetBool() );
    m_pCheck_SilhouetteVision->SetSelected( zm_cl_silhouette_onlyzmvision.GetBool() );
    m_pSlider_MaxRagdolls->SetValue( g_ragdoll_maxcount.GetInt() );


    char buffer[32];
    Q_snprintf( buffer, sizeof( buffer ), "%i", g_ragdoll_maxcount.GetInt() );

    m_pTextEntry_MaxRagdolls->SetText( buffer );
}

void CZMOptionsSubGraphics::OnSliderMoved( Panel* panel )
{
    if ( FailedLoad() ) return;


    if ( panel == m_pSlider_MaxRagdolls )
    {
        char buffer[32];
        Q_snprintf( buffer, sizeof( buffer ), "%i", m_pSlider_MaxRagdolls->GetValue() );

        m_pTextEntry_MaxRagdolls->SetText( buffer );
    }
}

void CZMOptionsSubGraphics::OnTextChanged( Panel* panel )
{
    if ( FailedLoad() ) return;


    if ( panel == m_pTextEntry_MaxRagdolls )
    {
        char buffer[32];
        m_pTextEntry_MaxRagdolls->GetText( buffer, sizeof( buffer ) );

        m_pSlider_MaxRagdolls->SetValue( atoi( buffer ) );
    }
}

