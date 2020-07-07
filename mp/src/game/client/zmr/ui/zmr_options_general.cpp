#include "cbase.h"
#include "filesystem.h"


#include "zmr_options_general.h"
#include "zmr_playermodels.h"
#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar zm_cl_usenewmenus;
extern ConVar zm_cl_zmview_switchmousebuttons;
extern ConVar zm_cl_poweruser;
extern ConVar zm_cl_poweruser_boxselect;

extern ConVar zm_cl_border_yawspeed;
extern ConVar zm_cl_border_pitchspeed;
extern ConVar zm_cl_border_scroll;

extern ConVar cl_playermodel;


CZMOptionsSubGeneral::CZMOptionsSubGeneral( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubgeneral.res" );

    LoadItem( &m_pPartBox, "ParticipationType" );
    LoadItem( &m_pCheck_NewMenus, "CheckNewMenus" );
    LoadItem( &m_pCheck_SwitchMouseBtns, "CheckSwitchMouseBtns" );
    LoadItem( &m_pCheck_PowerUser, "CheckPowerUser" );
    LoadItem( &m_pCheck_BoxPowerUser, "CheckPowerUserBox" );
    LoadItem( &m_pSlider_Yaw, "SliderScrollHor" );
    LoadItem( &m_pSlider_Pitch, "SliderScrollVer" );
    LoadItem( &m_pModelPanel, "CZMModelPanel1" );
    LoadItem( &m_pModelCombo, "ModelComboBox" );
    LoadItem( &m_pSlider_Border, "SliderBorder" );

    if ( FailedLoad() ) return;


    m_pSlider_Yaw->SetRange( 10, 500 );
    m_pSlider_Pitch->SetRange( 10, 500 );
    m_pSlider_Border->SetRange( 0, 100 );


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

    ZMGetPlayerModels()->LoadModelsFromFile();
    ZMPlayerModelList_t* pModels = ZMGetPlayerModels()->GetPlayerModels();
    for ( int i = 0; i < pModels->Count(); i++ )
    {
        CZMPlayerModelData* pData = pModels->Element( i );
        m_pModelCombo->AddItem( pData->GetModelData()->GetName(), pData->GetModelData() );
    }
}

CZMOptionsSubGeneral::~CZMOptionsSubGeneral()
{
}

void CZMOptionsSubGeneral::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    C_ZMPlayer::SetLocalParticipation( (Participation_t)m_pPartBox->GetActiveItem() );


    zm_cl_usenewmenus.SetValue( m_pCheck_NewMenus->IsSelected() ? 1 : 0 );

    zm_cl_zmview_switchmousebuttons.SetValue( m_pCheck_SwitchMouseBtns->IsSelected() ? 1 : 0 );

    zm_cl_poweruser.SetValue( m_pCheck_PowerUser->IsSelected() ? 1 : 0 );
    zm_cl_poweruser_boxselect.SetValue( m_pCheck_BoxPowerUser->IsSelected() ? 1 : 0 );


    zm_cl_border_yawspeed.SetValue( m_pSlider_Yaw->GetValue() );
    zm_cl_border_pitchspeed.SetValue( m_pSlider_Pitch->GetValue() );
    zm_cl_border_scroll.SetValue( m_pSlider_Border->GetValue() );

    cl_playermodel.SetValue( GetCurrentPlayerModel() );
}

void CZMOptionsSubGeneral::OnResetData()
{
    if ( FailedLoad() ) return;


    Participation_t part = C_ZMPlayer::GetLocalParticipation();

    m_pPartBox->ActivateItem( part );


    m_pCheck_NewMenus->SetSelected( zm_cl_usenewmenus.GetBool() );
    m_pCheck_SwitchMouseBtns->SetSelected( zm_cl_zmview_switchmousebuttons.GetBool() );
    m_pCheck_PowerUser->SetSelected( zm_cl_poweruser.GetBool() );
    m_pCheck_BoxPowerUser->SetSelected( zm_cl_poweruser_boxselect.GetBool() );


    m_pSlider_Yaw->SetValue( (int)zm_cl_border_yawspeed.GetFloat() );
    m_pSlider_Pitch->SetValue( (int)zm_cl_border_pitchspeed.GetFloat() );
    m_pSlider_Border->SetValue( (int)zm_cl_border_scroll.GetInt() );



    AppendCustomModels();

    bool bValidModel = false;

    const char* model = cl_playermodel.GetString();

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
        // Not the model we're looking for, pick a random one then.
        int count = m_pModelCombo->GetItemCount();
        m_pModelCombo->ActivateItemByRow( count ? random->RandomInt( 0, count - 1 ) : 0 );
    }
}

void CZMOptionsSubGeneral::AppendCustomModels()
{
    int i;
    ZMPlayerModelList_t list;

    ZMGetPlayerModels()->ParseCustomModels( list );

    // First remove all the old ones.
    // We might've changed servers.
    for ( i = 0; i < m_pModelCombo->GetItemCount(); i++ )
    {
        KeyValues* kv = m_pModelCombo->GetItemUserData( i );
        if ( kv && kv->GetBool( "custom" ) )
        {
            m_pModelCombo->DeleteItem( i );
            --i;
        }
    }

    // Add the new ones.
    for ( i = 0; i < list.Count(); i++ )
    {
        CZMPlayerModelData* pData = list[i];

        KeyValues* kv = pData->GetModelData();
        kv->SetBool( "custom", true );

        m_pModelCombo->AddItem( pData->GetModelData()->GetName(), kv );
    }

    if ( list.Count() )
    {
        DevMsg( "Added %i custom player models to model list.\n", list.Count() );
    }

    list.PurgeAndDeleteElements();
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
