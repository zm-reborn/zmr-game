#include "cbase.h"
#include "filesystem.h"
#include <tier0/icommandline.h>


#include "zmr_options_graphics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar zm_cl_glow_weapon_enabled;
extern ConVar zm_cl_glow_ammo_enabled;
extern ConVar zm_cl_zmvision_dlight;
extern ConVar zm_cl_silhouette_onlyzmvision;
extern ConVar g_ragdoll_maxcount;
extern ConVar zm_cl_precipitationquality;
extern ConVar zm_cl_flashlight_expensive_max;
extern ConVar zm_cl_colorcorrection_effects;
extern ConVar zm_cl_muzzleflash_light;


CZMOptionsSubGraphics::CZMOptionsSubGraphics( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubgraphics.res" );

    LoadItem( &m_pCheck_WepGlow, "CheckWeaponGlow" );
    LoadItem( &m_pCheck_AmmoGlow, "CheckAmmoGlow" );
    LoadItem( &m_pCheck_VisionDynLight, "CheckVisionDynLight" );
    LoadItem( &m_pCheck_SilhouetteVision, "CheckSilhouetteVision" );
    LoadItem( &m_pSlider_MaxRagdolls, "MaxRagdollsSlider" );
    LoadItem( &m_pTextEntry_MaxRagdolls, "MaxRagdollsEntry" );
    LoadItem( &m_pRainBox, "ComboRain" );
    LoadItem( &m_pExpFlashlightAmtBox, "ComboFlashlightAmount" );
    LoadItem( &m_pCheck_CC, "CheckCC" );
    LoadItem( &m_pMuzzleflashBox, "ComboMuzzleflash" );


    if ( FailedLoad() ) return;


    auto* tempkv = new KeyValues( "temp" );

    m_pRainBox->AddItem( L"None", tempkv ); // 0
    m_pRainBox->AddItem( L"Low", tempkv ); // 1
    m_pRainBox->AddItem( L"Medium", tempkv ); // 2
    m_pRainBox->AddItem( L"High", tempkv ); // 3

    m_pExpFlashlightAmtBox->AddItem( L"0", tempkv );
    m_pExpFlashlightAmtBox->AddItem( L"1", tempkv );
    m_pExpFlashlightAmtBox->AddItem( L"2", tempkv );
    m_pExpFlashlightAmtBox->AddItem( L"3", tempkv );

    m_pMuzzleflashBox->AddItem( L"None", tempkv );
    m_pMuzzleflashBox->AddItem( L"Local Only", tempkv );
    m_pMuzzleflashBox->AddItem( L"All Players", tempkv );

    tempkv->deleteThis();

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
    zm_cl_colorcorrection_effects.SetValue( m_pCheck_CC->IsSelected() ? 1 : 0 );


    zm_cl_flashlight_expensive_max.SetValue( m_pExpFlashlightAmtBox->GetActiveItem() );
    zm_cl_muzzleflash_light.SetValue( m_pMuzzleflashBox->GetActiveItem() );

    int rainval = m_pRainBox->GetActiveItem();
    switch ( rainval )
    {
    case 0 :
    case 1 :
    case 2 : // None/Low/Medium
        zm_cl_precipitationquality.SetValue( rainval );
        break;
    case 3 : // High
    default :
        zm_cl_precipitationquality.SetValue( 3 );
        break;
    }
}

void CZMOptionsSubGraphics::OnResetData()
{
    if ( FailedLoad() ) return;


    m_pCheck_WepGlow->SetSelected( zm_cl_glow_weapon_enabled.GetBool() );
    m_pCheck_AmmoGlow->SetSelected( zm_cl_glow_ammo_enabled.GetBool() );
    m_pCheck_VisionDynLight->SetSelected( zm_cl_zmvision_dlight.GetBool() );
    m_pCheck_SilhouetteVision->SetSelected( zm_cl_silhouette_onlyzmvision.GetBool() );
    m_pSlider_MaxRagdolls->SetValue( g_ragdoll_maxcount.GetInt() );
    m_pCheck_CC->SetSelected( zm_cl_colorcorrection_effects.GetBool() );



    m_pExpFlashlightAmtBox->ActivateItem( zm_cl_flashlight_expensive_max.GetInt() );
    m_pMuzzleflashBox->ActivateItem( zm_cl_muzzleflash_light.GetInt() );
    
    int rainval = zm_cl_precipitationquality.GetInt();
    switch ( rainval )
    {
    case 0 :
    case 1 :
    case 2 : // None/Low/Medium
        m_pRainBox->ActivateItem( rainval );
        break;
    case 3 : // High
    default :
        m_pRainBox->ActivateItem( 3 );
        break;
    }


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

