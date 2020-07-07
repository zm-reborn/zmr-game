#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_misc.h"
#include "c_zmr_player.h"


extern ConVar zm_cl_firstperson_deathcam;
extern ConVar zm_cl_deathnotice_enabled;
extern ConVar zm_cl_showhelp;
extern ConVar zm_cl_fov;
extern ConVar zm_cl_roundrestart_flashtaskbar;
extern ConVar zm_cl_roundrestart_sound;


static int RoundRestartCvarsToIndex();
static void IndexToRoundRestartCvars( int index );


CZMOptionsSubMisc::CZMOptionsSubMisc( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubmisc.res" );


    LoadItem( &m_pCheck_FpDeathcam, "CheckFpDeathcam" );
    LoadItem( &m_pCheck_Deathnotice, "CheckDeathnotice" );
    LoadItem( &m_pCheck_ShowHelp, "CheckShowHelp" );
    LoadItem( &m_pSlider_Fov, "SliderFov" );
    LoadItem( &m_pTextEntry_Fov, "FovEntry" );
    LoadItem( &m_pCombo_FlashRR, "FlashRoundRestartCombo" );

    if ( FailedLoad() ) return;


    float fmin, fmax;

    if ( zm_cl_fov.GetMin( fmin ) && zm_cl_fov.GetMax( fmax ) )
        m_pSlider_Fov->SetRange( fmin, fmax );

    m_pSlider_Fov->AddActionSignalTarget( this );


    m_pCombo_FlashRR->AddItem( "None", nullptr );
    m_pCombo_FlashRR->AddItem( "Play sound", nullptr );
    m_pCombo_FlashRR->AddItem( "Flash taskbar", nullptr );
    m_pCombo_FlashRR->AddItem( "Play sound + flash taskbar", nullptr );
    
#ifdef LINUX
    m_pCombo_FlashRR->SetEnabled( false );
#endif
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
    IndexToRoundRestartCvars( m_pCombo_FlashRR->GetActiveItem() );
}

void CZMOptionsSubMisc::OnResetData()
{
    if ( FailedLoad() ) return;


    m_pCheck_FpDeathcam->SetSelected( zm_cl_firstperson_deathcam.GetBool() );
    m_pCheck_Deathnotice->SetSelected( zm_cl_deathnotice_enabled.GetBool() );
    m_pCheck_ShowHelp->SetSelected( zm_cl_showhelp.GetBool() );
    m_pSlider_Fov->SetValue( zm_cl_fov.GetInt() );
    m_pCombo_FlashRR->ActivateItem( RoundRestartCvarsToIndex() );
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


static int RoundRestartCvarsToIndex()
{
    if ( zm_cl_roundrestart_flashtaskbar.GetBool() && zm_cl_roundrestart_sound.GetBool() )
    {
        return 3;
    }
    else if ( zm_cl_roundrestart_flashtaskbar.GetBool() )
    {
        return 2;
    }
    else if ( zm_cl_roundrestart_sound.GetBool() )
    {
        return 1;
    }

    return 0;
}

static void IndexToRoundRestartCvars( int index )
{
    switch ( index )
    {
    case 0 :
        zm_cl_roundrestart_flashtaskbar.SetValue( 0 );
        zm_cl_roundrestart_sound.SetValue( 0 );
        break;
    case 1 :
        zm_cl_roundrestart_flashtaskbar.SetValue( 0 );
        zm_cl_roundrestart_sound.SetValue( 1 );
        break;
    case 2 :
        zm_cl_roundrestart_flashtaskbar.SetValue( 1 );
        zm_cl_roundrestart_sound.SetValue( 0 );
        break;
    case 3 :
    default :
        zm_cl_roundrestart_flashtaskbar.SetValue( 1 );
        zm_cl_roundrestart_sound.SetValue( 1 );
        break;
    }
}
