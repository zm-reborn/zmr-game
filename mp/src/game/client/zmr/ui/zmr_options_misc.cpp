#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_misc.h"
#include "zmr/c_zmr_player.h"


extern ConVar zm_cl_firstperson_deathcam;
extern ConVar zm_cl_deathnotice_enabled;


CZMOptionsSubMisc::CZMOptionsSubMisc( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubmisc.res" );


    LoadItem( &m_pCheck_FpDeathcam, "CheckFpDeathcam" );
    LoadItem( &m_pCheck_Deathnotice, "CheckDeathnotice" );
}

CZMOptionsSubMisc::~CZMOptionsSubMisc()
{
}

void CZMOptionsSubMisc::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    zm_cl_firstperson_deathcam.SetValue( m_pCheck_FpDeathcam->IsSelected() ? 1 : 0 );
    zm_cl_deathnotice_enabled.SetValue( m_pCheck_Deathnotice->IsSelected() ? 1 : 0 );
}

void CZMOptionsSubMisc::OnResetData()
{
    if ( FailedLoad() ) return;


    m_pCheck_FpDeathcam->SetSelected( zm_cl_firstperson_deathcam.GetBool() );
    m_pCheck_Deathnotice->SetSelected( zm_cl_deathnotice_enabled.GetBool() );
}
