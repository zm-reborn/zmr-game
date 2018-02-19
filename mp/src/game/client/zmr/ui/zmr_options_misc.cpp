#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_misc.h"
#include "zmr/c_zmr_player.h"


extern ConVar zm_cl_firstperson_deathcam;
extern ConVar zm_cl_deathnotice_enabled;
extern ConVar zm_cl_showhelp;
extern ConVar zm_cl_fov;


CZMOptionsSubMisc::CZMOptionsSubMisc( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubmisc.res" );


    LoadItem( &m_pCheck_FpDeathcam, "CheckFpDeathcam" );
    LoadItem( &m_pCheck_Deathnotice, "CheckDeathnotice" );
    LoadItem( &m_pCheck_ShowHelp, "CheckShowHelp" );
    LoadItem( &m_pSlider_Fov, "SliderFov" );
    LoadItem( &m_pTextEntry_Fov, "FovEntry" );

    if ( FailedLoad() ) return;


    float fmin, fmax;

    if ( zm_cl_fov.GetMin( fmin ) && zm_cl_fov.GetMax( fmax ) )
        m_pSlider_Fov->SetRange( fmin, fmax );

    m_pSlider_Fov->AddActionSignalTarget( this );
}

CZMOptionsSubMisc::~CZMOptionsSubMisc()
{
}

void CZMOptionsSubMisc::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    zm_cl_firstperson_deathcam.SetValue( m_pCheck_FpDeathcam->IsSelected() ? 1 : 0 );
    zm_cl_deathnotice_enabled.SetValue( m_pCheck_Deathnotice->IsSelected() ? 1 : 0 );
    zm_cl_showhelp.SetValue( m_pCheck_ShowHelp->IsSelected() ? 1 : 0 );
    zm_cl_fov.SetValue( m_pSlider_Fov->GetValue() );
}

void CZMOptionsSubMisc::OnResetData()
{
    if ( FailedLoad() ) return;


    m_pCheck_FpDeathcam->SetSelected( zm_cl_firstperson_deathcam.GetBool() );
    m_pCheck_Deathnotice->SetSelected( zm_cl_deathnotice_enabled.GetBool() );
    m_pCheck_ShowHelp->SetSelected( zm_cl_showhelp.GetBool() );
    m_pSlider_Fov->SetValue( zm_cl_fov.GetInt() );
}

void CZMOptionsSubMisc::OnSliderMoved( Panel* panel )
{
    if ( FailedLoad() ) return;


    if ( panel == m_pSlider_Fov )
    {
        char buffer[32];
        Q_snprintf( buffer, sizeof( buffer ), "%i", m_pSlider_Fov->GetValue() );

        m_pTextEntry_Fov->SetText( buffer );
    }
}

void CZMOptionsSubMisc::OnTextChanged( Panel* panel )
{
    if ( FailedLoad() ) return;


    if ( panel == m_pTextEntry_Fov )
    {
        char buffer[32];
        m_pTextEntry_Fov->GetText( buffer, sizeof( buffer ) );

        m_pSlider_Fov->SetValue( atoi( buffer ) );
    }
}
