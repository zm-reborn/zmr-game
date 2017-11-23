#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_general.h"
#include "zmr/c_zmr_player.h"


extern ConVar zm_cl_usenewmenus;
extern ConVar zm_cl_poweruser;
extern ConVar zm_cl_poweruser_boxselect;

extern ConVar cl_yawspeed;
extern ConVar cl_pitchspeed;


CZMOptionsSubGeneral::CZMOptionsSubGeneral( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubgeneral.res" );

    LoadItem( &m_pPartBox, "ParticipationType" );
    LoadItem( &m_pCheck_NewMenus, "CheckNewMenus" );
    LoadItem( &m_pCheck_PowerUser, "CheckPowerUser" );
    LoadItem( &m_pCheck_BoxPowerUser, "CheckPowerUserBox" );
    LoadItem( &m_pSlider_Yaw, "SliderScrollHor" );
    LoadItem( &m_pSlider_Pitch, "SliderScrollVer" );
    LoadItem( &m_pModelPanel, "CZMModelPanel1" );
    LoadItem( &m_pModelCombo, "ModelComboBox" );

    if ( FailedLoad() ) return;


    m_pSlider_Yaw->SetRange( 10, 500 );
    m_pSlider_Pitch->SetRange( 10, 500 );


    KeyValues* kv;

    kv = new KeyValues( "participation" );

    if ( kv->LoadFromFile( filesystem, "resource/zmoptions_participation.txt", "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();

        while ( pKey )
        {
            m_pPartBox->AddItem( pKey->GetString( "label", "" ), pKey );

            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load participation values from file!\n" );
    }

    kv->deleteThis();

    m_pPartBox->ActivateItemByRow( 0 );




    m_pModelPanel->SetPanelDirty();
    m_pModelCombo->ActivateItemByRow( 0 );

    kv = new KeyValues( "Models" );

    if ( kv->LoadFromFile( filesystem, "resource/zmoptions_playermodels.txt", "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();

        while ( pKey )
        {
            m_pModelCombo->AddItem( pKey->GetName(), pKey );

            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load player models from file!\n" );
    }

    

    kv->deleteThis();
}

CZMOptionsSubGeneral::~CZMOptionsSubGeneral()
{
}

void CZMOptionsSubGeneral::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    C_ZMPlayer::SetLocalParticipation( (Participation_t)m_pPartBox->GetActiveItem() );


    zm_cl_usenewmenus.SetValue( m_pCheck_NewMenus->IsSelected() ? 1 : 0 );

    zm_cl_poweruser.SetValue( m_pCheck_PowerUser->IsSelected() ? 1 : 0 );
    zm_cl_poweruser_boxselect.SetValue( m_pCheck_BoxPowerUser->IsSelected() ? 1 : 0 );


    cl_yawspeed.SetValue( m_pSlider_Yaw->GetValue() );
    cl_pitchspeed.SetValue( m_pSlider_Pitch->GetValue() );

    ConVar* cl_playermodel = cvar->FindVar( "cl_playermodel" );
    cl_playermodel->SetValue( GetCurrentPlayerModel() );
}

void CZMOptionsSubGeneral::OnResetData()
{
    if ( FailedLoad() ) return;


    Participation_t part = C_ZMPlayer::GetLocalParticipation();

    m_pPartBox->ActivateItem( part );


    m_pCheck_NewMenus->SetSelected( zm_cl_usenewmenus.GetBool() );
    m_pCheck_PowerUser->SetSelected( zm_cl_poweruser.GetBool() );
    m_pCheck_BoxPowerUser->SetSelected( zm_cl_poweruser_boxselect.GetBool() );


    m_pSlider_Yaw->SetValue( (int)cl_yawspeed.GetFloat() );
    m_pSlider_Pitch->SetValue( (int)cl_pitchspeed.GetFloat() );


    bool bValidModel = false;

    ConVar* cl_playermodel = cvar->FindVar( "cl_playermodel" );
    const char* model = cl_playermodel->GetString();

    for ( int i = 0; i < m_pModelCombo->GetItemCount(); i++ )
    {
        KeyValues* kv = m_pModelCombo->GetItemUserData( i );

        if ( Q_stricmp( model, kv->GetString( "model" ) ) == 0 )
        {
            m_pModelCombo->ActivateItem( i );
            bValidModel = true;
        }
    }

    if ( !bValidModel )
    {
        m_pModelCombo->ActivateItemByRow( 0 );
    }
}


const char* CZMOptionsSubGeneral::GetCurrentPlayerModel()
{
    KeyValues* kv = m_pModelCombo->GetActiveItemUserData();

    return kv->GetString( "model" );
}

void CZMOptionsSubGeneral::OnComboChanged( KeyValues* kv )
{
    if ( !FailedLoad() )
    {
        m_pModelPanel->SwapModel( GetCurrentPlayerModel() );
    }
}
