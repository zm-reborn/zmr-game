#include "cbase.h"

#include "zmr/c_zmr_crosshair.h"
#include "zmr_options_crosshair.h"


CZMOptionsSubCrosshair::CZMOptionsSubCrosshair( Panel* parent ) : BaseClass( parent )
{
    LoadControlSettings( "resource/ui/zmoptionssubcrosshair.res" );


    LoadItem( &m_pCrosshairPanel, "CrosshairDraw" );
    LoadItem( &m_pCrosshairCombo, "CrosshairCombo" );
    LoadItem( &m_pColorRed, "ColorRed" );
    LoadItem( &m_pColorGreen, "ColorGreen" );
    LoadItem( &m_pColorBlue, "ColorBlue" );
    LoadItem( &m_pColorAlpha, "ColorAlpha" );
    LoadItem( &m_pOutlineColorRed, "OutlineColorRed" );
    LoadItem( &m_pOutlineColorGreen, "OutlineColorGreen" );
    LoadItem( &m_pOutlineColorBlue, "OutlineColorBlue" );
    LoadItem( &m_pOutlineColorAlpha, "OutlineColorAlpha" );
    LoadItem( &m_pOutlineSize, "OutlineSize" );
    LoadItem( &m_pCrossType, "CrossType" );
    LoadItem( &m_pDynMove, "DynMove" );
    LoadItem( &m_pOffsetFromCenter, "OffsetFromCenter" );

    if ( FailedLoad() ) return;


    KeyValues* tempkv = new KeyValues( "data" );
    CUtlVector<CZMBaseCrosshair*>* crosses = g_ZMCrosshairs.GetCrosshairs();
    for ( int i = 0; i < crosses->Count(); i++ )
    {
        if ( !crosses->Element( i )->DisplayInMenu() )
            continue;


        KeyValues* kv = new KeyValues( "Crosshairs" );
        KeyValues* crossdata = kv->FindKey( STRING( crosses->Element( i )->GetName() ), true );
        crosses->Element( i )->WriteValues( crossdata );

        CZMBaseCrosshair* pOurCross = CZMCrosshairSystem::CreateCrosshairFromData( crossdata );
        if ( pOurCross )
        {
            pOurCross->LoadValues( crossdata );

            int index = m_vCrosshairs.AddToTail( pOurCross );
            tempkv->SetInt( "index", index );
            m_pCrosshairCombo->AddItem( crosses->Element( i )->GetMenuName(), tempkv );
        }

        kv->deleteThis();
    }

    tempkv->deleteThis();

    if ( m_vCrosshairs.IsValidIndex( 0 ) )
    {
        m_pCrosshairCombo->ActivateItemByRow( 0 );
        m_pCrosshairPanel->SetCrosshairToDraw( m_vCrosshairs[0] );
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

}

void CZMOptionsSubCrosshair::OnResetData()
{
    if ( FailedLoad() ) return;

}

void CZMOptionsSubCrosshair::OnComboChanged( KeyValues* kv )
{
    if ( !FailedLoad() )
    {
        KeyValues* kv = m_pCrosshairCombo->GetActiveItemUserData();
        int i = kv->GetInt( "index", -1 );

        if ( m_vCrosshairs.IsValidIndex( i ) )
        {
            

            m_pCrosshairPanel->SetCrosshairToDraw( m_vCrosshairs[i] );
        }
        else
        {
            UpdateCrosshair( i );
        }
    }
}

void CZMOptionsSubCrosshair::UpdateSettings( int index )
{
    if ( FailedLoad() )
        return;


    CZMBaseCrosshair* pCross = m_vCrosshairs[index];


    KeyValues* kv = new KeyValues( "data" );

    pCross->WriteValues( kv );

    char temp[64];
    Color clr;

    clr = kv->GetColor( "color" );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[0] );
    m_pColorRed->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[1] );
    m_pColorGreen->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[2] );
    m_pColorBlue->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[3] );
    m_pColorAlpha->SetText( temp );


    clr = kv->GetColor( "outlinecolor" );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[0] );
    m_pOutlineColorRed->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[1] );
    m_pOutlineColorGreen->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[2] );
    m_pOutlineColorBlue->SetText( temp );
    Q_snprintf( temp, sizeof( temp ), "%i", clr[3] );
    m_pOutlineColorAlpha->SetText( temp );

    Q_snprintf( temp, sizeof( temp ), "%i", kv->GetFloat( "outline" ) );
    m_pOutlineSize->SetText( temp );



    pCross->LoadValues( kv );



    kv->deleteThis();
}

void CZMOptionsSubCrosshair::UpdateCrosshair( int index )
{
    if ( FailedLoad() )
        return;


    CZMBaseCrosshair* pCross = m_vCrosshairs[index];


    KeyValues* kv = new KeyValues( STRING( pCross->GetName() ) );

    pCross->WriteValues( kv );


    Color clr;

    

    clr[0] = m_pColorRed->GetValueAsInt();
    clr[1] = m_pColorGreen->GetValueAsInt();
    clr[2] = m_pColorBlue->GetValueAsInt();
    clr[3] = m_pColorAlpha->GetValueAsInt();

    kv->SetColor( "color", clr );


    clr[0] = m_pOutlineColorRed->GetValueAsInt();
    clr[1] = m_pOutlineColorGreen->GetValueAsInt();
    clr[2] = m_pOutlineColorBlue->GetValueAsInt();
    clr[3] = m_pOutlineColorAlpha->GetValueAsInt();
    kv->SetColor( "outlinecolor", clr );


    kv->SetFloat( "outline", m_pOutlineSize->GetValueAsInt() );




    pCross->LoadValues( kv );



    kv->deleteThis();
}

