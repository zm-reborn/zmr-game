#include "cbase.h"

#include "zmr/c_zmr_crosshair.h"
#include "zmr_options_crosshair.h"


CZMOptionsSubCrosshair::CZMOptionsSubCrosshair( Panel* parent ) : BaseClass( parent )
{
    m_iCurCrosshair = -1;


    LoadControlSettings( "resource/ui/zmoptionssubcrosshair.res" );


    LoadItem( &m_pCrosshairPanel, "CrosshairDraw" );
    LoadItem( &m_pCrosshairCombo, "CrosshairCombo" );
    LoadItem( &m_pSlider_ColorR, "SliderColorR" );
    LoadItem( &m_pSlider_ColorG, "SliderColorG" );
    LoadItem( &m_pSlider_ColorB, "SliderColorB" );
    LoadItem( &m_pSlider_ColorA, "SliderColorA" );
    LoadItem( &m_pOutlineSize, "OutlineSize" );
    LoadItem( &m_pDotSize, "DotSize" );
    LoadItem( &m_pDynMove, "DynMove" );
    LoadItem( &m_pOffsetFromCenter, "OffsetFromCenter" );

    if ( FailedLoad() ) return;


    m_pSlider_ColorR->SetRange( 0, 255 );
    m_pSlider_ColorG->SetRange( 0, 255 );
    m_pSlider_ColorB->SetRange( 0, 255 );
    m_pSlider_ColorA->SetRange( 0, 255 );


    // Copy the crosshairs, so we can safely edit the values without changing the real in-game crosshair.
    KeyValues* tempkv = new KeyValues( "data" );
    CUtlVector<CZMBaseCrosshair*>* crosses = g_ZMCrosshairs.GetCrosshairs();
    for ( int i = 0; i < crosses->Count(); i++ )
    {
        KeyValues* crossdata = new KeyValues( crosses->Element( i )->GetName() );
        crosses->Element( i )->WriteValues( crossdata );

        CZMBaseCrosshair* pOurCross = CZMCrosshairSystem::CreateCrosshairFromData( crossdata );
        if ( pOurCross )
        {
            pOurCross->LoadValues( crossdata );

            int index = m_vCrosshairs.AddToTail( pOurCross );
            tempkv->SetInt( "crosshairindex", index );

            if ( pOurCross->DisplayInMenu() )
            {
                m_pCrosshairCombo->AddItem( crosses->Element( i )->GetMenuName(), tempkv );
            }
        }

        crossdata->deleteThis();
    }

    tempkv->deleteThis();


    // Init the panel
    if ( m_vCrosshairs.IsValidIndex( 0 ) )
    {
        m_pCrosshairCombo->ActivateItemByRow( 0 );
        m_pCrosshairPanel->SetCrosshairToDraw( m_vCrosshairs[0] );


        m_iCurCrosshair = 0;
        UpdateSettings();
    }
}

CZMOptionsSubCrosshair::~CZMOptionsSubCrosshair()
{
    if ( m_pCrosshairPanel )
    {
        m_pCrosshairPanel->SetCrosshairToDraw( nullptr );
        m_pCrosshairPanel = nullptr;
    }

    m_vCrosshairs.PurgeAndDeleteElements();
}

void CZMOptionsSubCrosshair::OnApplyChanges()
{
    if ( FailedLoad() ) return;


    // We want these changes, copy our data over to the in-game crosshairs.
    CUtlVector<CZMBaseCrosshair*>* crosses = g_ZMCrosshairs.GetCrosshairs();
    Assert( crosses->Count() == m_vCrosshairs.Count() );


    FOR_EACH_VEC( m_vCrosshairs, i )
    {
        KeyValues* tempkv = new KeyValues( m_vCrosshairs[i]->GetName() );

        m_vCrosshairs[i]->WriteValues( tempkv );

        crosses->Element( i )->LoadValues( tempkv );

        tempkv->deleteThis();
    }

    // Write the changes to our config file.
    g_ZMCrosshairs.WriteCrosshairsToFile();
}

void CZMOptionsSubCrosshair::OnResetData()
{
    if ( FailedLoad() ) return;

    

    // Reset our to the in-game equivalent

    CUtlVector<CZMBaseCrosshair*>* crosses = g_ZMCrosshairs.GetCrosshairs();
    if ( crosses->Count() != m_vCrosshairs.Count() )
        return;


    for ( int i = 0; i < crosses->Count(); i++ )
    {
        KeyValues* tempkv = new KeyValues( crosses->Element( i )->GetName() );

        crosses->Element( i )->WriteValues( tempkv );

        m_vCrosshairs[i]->LoadValues( tempkv );

        tempkv->deleteThis();
    }
}

void CZMOptionsSubCrosshair::OnComboChanged( Panel* panel )
{
    if ( FailedLoad() )
        return;


    // Something changed...

    if ( panel == m_pCrosshairCombo )
    {
        KeyValues* mykv = m_pCrosshairCombo->GetActiveItemUserData();

        int i = mykv->GetInt( "crosshairindex", -1 );

        if ( m_iCurCrosshair != i && m_vCrosshairs.IsValidIndex( i ) )
        {
            m_pCrosshairPanel->SetCrosshairToDraw( m_vCrosshairs[i] );
            m_iCurCrosshair = i;

            UpdateSettings();
        }

        return;
    }



    UpdateCrosshair();
}

void CZMOptionsSubCrosshair::OnSliderMoved( Panel* panel )
{
    if ( FailedLoad() ) return;


    // Something changed...

    UpdateCrosshair();
}

void CZMOptionsSubCrosshair::UpdateSettings( int index )
{
    if ( FailedLoad() )
        return;

    if ( !m_vCrosshairs.IsValidIndex( index ) )
    {
        index = m_iCurCrosshair;
        if ( !m_vCrosshairs.IsValidIndex( index ) ) return;
    }


    // Update the ui elements to fit the crosshair data.


    CZMBaseCrosshair* pCross = m_vCrosshairs[index];


    KeyValues* kv = new KeyValues( pCross->GetName() );
    pCross->WriteValues( kv );


    Color clr;

    clr = kv->GetColor( "color" );
    m_pSlider_ColorR->SetValue( clr[0], false );
    m_pSlider_ColorG->SetValue( clr[1], false );
    m_pSlider_ColorB->SetValue( clr[2], false );
    m_pSlider_ColorA->SetValue( clr[3], false );


    m_pOutlineSize->SetValue( kv->GetFloat( "outline" ), false );

    m_pDotSize->SetValue( kv->GetFloat( "dot" ), false );

    m_pOffsetFromCenter->SetValue( kv->GetFloat( "offsetfromcenter" ), false );

    m_pDynMove->SetValue( kv->GetFloat( "dynamicmove" ), false );

    pCross->LoadValues( kv );



    kv->deleteThis();
}

void CZMOptionsSubCrosshair::UpdateCrosshair( int index )
{
    if ( FailedLoad() )
        return;

    if ( !m_vCrosshairs.IsValidIndex( index ) )
    {
        index = m_iCurCrosshair;
        if ( !m_vCrosshairs.IsValidIndex( index ) ) return;
    }


    // Update the display crosshair to fit the settings.

    CZMBaseCrosshair* pCross = m_vCrosshairs[index];


    KeyValues* kv = new KeyValues( pCross->GetName() );
    pCross->WriteValues( kv );


    Color clr;


    clr[0] = m_pSlider_ColorR->GetValue();
    clr[1] = m_pSlider_ColorG->GetValue();
    clr[2] = m_pSlider_ColorB->GetValue();
    clr[3] = m_pSlider_ColorA->GetValue();

    kv->SetColor( "color", clr );

    kv->SetFloat( "outline", m_pOutlineSize->GetValue() );

    kv->SetFloat( "dot", m_pDotSize->GetValue() );

    kv->SetFloat( "offsetfromcenter", m_pOffsetFromCenter->GetValue() );
    kv->SetFloat( "dynamicmove", m_pDynMove->GetValue() );


    pCross->LoadValues( kv );



    kv->deleteThis();
}

