#include "cbase.h"
#include <vgui/ILocalize.h>

#include "zmr_imagerow.h"
#include "zmr_radial.h"
#include "zmr_buildmenu_new.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


static void SortZMQueueImages( Panel* pPanel, CUtlVector<CZMPlaceImage*>* pImages )
{
    int size = pPanel->GetWide() > pPanel->GetTall() ? pPanel->GetTall() : pPanel->GetWide();
    int cx = size / 2;
    int cy = size / 2;
    size = size / 2 - 40;

    float dir = -90.0f;
    float jump = 360.0f / 10.0f; // Update if queue sizes can be changed.
    for ( int i = 0; i < pImages->Count(); i++ )
    {
        CZMPlaceImage* pImage = pImages->Element( i );

        float vx, vy;
        int w, h;

        float rad = DEG2RAD( dir );
        vx = cos( rad );
        vy = sin( rad );

        pImage->GetSize( w, h );
        pImage->SetPos( cx + vx * size - w / 2, cy + vy * size - h / 2 );

        dir = AngleNormalize( dir + jump );
    }
}


CZMBuildMenuNew::CZMBuildMenuNew( Panel* pParent ) : CZMBuildMenuBase( "ZMBuildMenuNew" )
{
    SetParent( pParent->GetVPanel() );

    SetPaintBackgroundEnabled( false );
    SetSizeable( false );
	SetProportional( true );
	SetMoveable( false );
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );
    DisableMouseInputForThisPanel( true ); // Only THIS panel can't be clicked. Children work fine.


    m_iCurTooltip = -1;

	LoadControlSettings( "resource/ui/zmbuildmenunew.res" );


    m_pRadial = dynamic_cast<CZMRadialPanel*>( FindChildByName( "ZMRadialPanel1" ) );
    Assert( m_pRadial );
    m_pRadial->LoadFromFile( "resource/zmradial_spawn.txt" );
    m_pRadial->SetBackgroundImage( "zmr_buildmenu/bg" );

    m_pImageList = dynamic_cast<CZMImageRow*>( FindChildByName( "ZMQueueImages1" ) );
    Assert( m_pImageList );
    m_pImageList->SetImagesSize( 32 );
    m_pImageList->SetLayoutFunc( SortZMQueueImages );


    const char* queueImages[ZMCLASS_MAX] = {
        "zombies/queue_shambler",
        "zombies/queue_banshee",
        "zombies/queue_hulk",
        "zombies/queue_drifter",
        "zombies/queue_immolator",
    };

    for ( int i = 0; i < ZMCLASS_MAX; i++ )
    {
        m_pQueueImages[i] = vgui::scheme()->GetImage( queueImages[i], true );
    }
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CZMBuildMenuNew::~CZMBuildMenuNew()
{
}

void CZMBuildMenuNew::ShowPanel( bool state )
{
    if ( IsVisible() == state ) return;

    
    if ( state )
    {
        // Notify the server we've opened this menu.
        engine->ClientCmd( VarArgs( "zm_cmd_openbuildmenu %i", GetSpawnIndex() ) );
    }


    SetVisible( state );
}

void CZMBuildMenuNew::OnThink()
{
	if ( !IsVisible() ) return;


    MoveToFront();


    // Update world position.
    int x, y;
    GetPos( x, y );
    GetScreenPos( x, y );

    SetPos( x, y );


    UpdateButtons();
}

void CZMBuildMenuNew::UpdateButtons()
{
    static bool disabled[ZMCLASS_MAX];
    int i, j;
    int flags = GetZombieFlags();


    // ZMRTODO: Add resource check.
    for ( i = 0; i < ZMCLASS_MAX; i++ )
    {
        if ( flags == 0 )
            disabled[i] = false;
        else
        {
            disabled[i] = ( flags & (1 << i) ) ? false : true;
        }
    }
    

    // HACK: Hardcode the first 10 buttons. Make sure they are also in the right order.
    CUtlVector<CZMRadialButton*>* pButtons = m_pRadial->GetButtons();
    Assert( pButtons->Count() >= ((int)ZMCLASS_MAX * 2) );

    for ( i = 0; i < (int)ZMCLASS_MAX * 2; i++ )
    {
        j = i % (int)ZMCLASS_MAX;

        pButtons->Element( i )->SetDisabled( disabled[j] );
    }
}

void CZMBuildMenuNew::OnImageRowPressed( KeyValues* kv )
{
    // Clear the specific pos on the queue.
    int index = kv->GetInt( "index", -1 );
    
    if ( m_pImageList->GetImageByIndex( index ) != nullptr )
    {
        char buffer[128];
        Q_snprintf( buffer, sizeof( buffer ), "zm_cmd_queueclear %%i 1 %i", index + 1 );
        OnCommand( buffer );
    }
}

void CZMBuildMenuNew::OnRadialOver( KeyValues* kv )
{
    CZMRadialButton* pButton = static_cast<CZMRadialButton*>( kv->GetPtr( "button" ) );
    
    if ( pButton )
    {
        if ( pButton->GetLabel() )
        {
            // Update pop and cost values.
            const char* szClass = pButton->GetLabelData() ? pButton->GetLabelData()->GetString( "zombieclass" ) : "";

            if ( *szClass )
            {
                ZombieClass_t zclass = C_ZMBaseZombie::NameToClass( szClass );
                const char* format = g_pVGuiLocalize->FindAsUTF8( "#ZMRadialMouseOver" );
                if ( format )
                {
                    char buffer[128];
                    Q_snprintf( buffer, sizeof( buffer ), format, C_ZMBaseZombie::GetPopCost( zclass ), C_ZMBaseZombie::GetCost( zclass ) );
                    pButton->GetLabel()->SetText( buffer );
                }
            }

            pButton->GetLabel()->SetVisible( true );
        }

        const char* szTooltip = pButton->GetLabelData() ? pButton->GetLabelData()->GetString( "tooltip" ) : "";
        if ( *szTooltip )
        {
            m_iCurTooltip = ZMClientUtil::ShowTooltipByName( szTooltip );
        }
    }
}

void CZMBuildMenuNew::OnRadialLeave( KeyValues* kv )
{
    CZMRadialButton* pButton = static_cast<CZMRadialButton*>( kv->GetPtr( "button" ) );

    if ( pButton )
    {
        if ( pButton->GetLabel() )
            pButton->GetLabel()->SetVisible( false );
    }

    if ( m_iCurTooltip != -1 )
    {
        ZMClientUtil::HideTooltip( m_iCurTooltip );
        m_iCurTooltip = -1;
    }
}

void CZMBuildMenuNew::ShowMenu( C_ZMEntZombieSpawn* pSpawn )
{
    SetWorldPos( pSpawn->GetAbsOrigin() );

    if ( m_pRadial )
    {
        float frac = m_pRadial->GetWide() * 0.1f;
        SetOffset(
            m_pRadial->GetXPos() + m_pRadial->GetWide() / 2,
            m_pRadial->GetYPos() + m_pRadial->GetTall() / 2 );
        SetLimits(
            m_pRadial->GetXPos() + frac,
            m_pRadial->GetYPos() + frac,
            m_pRadial->GetXPos() + m_pRadial->GetWide() - frac,
            m_pRadial->GetYPos() + m_pRadial->GetTall() - frac );
    }


    BaseClass::ShowMenu( pSpawn );
}

void CZMBuildMenuNew::UpdateQueue( const int queue[], int size )
{
    // Recreate the queue.
    m_pImageList->RemoveImages();

    for ( int i = 0; i < size; i++ )
    {
		const ZombieClass_t type = (ZombieClass_t)queue[i];

		if ( C_ZMBaseZombie::IsValidClass( type ) )
		{
			IImage* pImg = m_pQueueImages[(int)type];
            int w = m_pImageList->GetImagesSize();
            pImg->SetSize( w, w );
            m_pImageList->AddImage( pImg );
		}
    }

    m_pImageList->PerformLayout();
}
