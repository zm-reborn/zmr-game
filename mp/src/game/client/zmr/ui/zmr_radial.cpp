#include "cbase.h"
#include "filesystem.h"
#include "input.h"
#include <vgui/ISurface.h>


#include "zmr_imagerow.h"
#include "zmr_radial.h"


using namespace vgui;


CZMRadialButton::CZMRadialButton( Panel* parent, const char* name ) : Panel( parent, name )
{
    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( false );
    SetKeyBoardInputEnabled( false );


    m_pCurImage = nullptr;
    m_pImage = nullptr;
    m_pImageFocus = nullptr;
    m_pImageDisabled = nullptr;
    m_pszCommand = nullptr;

    m_bDisabled = false;

    m_pTextLabel = new Label( this, "", "" );
    m_pTextLabel->SetProportional( true );//parent ? parent->IsProportional() : true );
    m_pTextLabel->SetPaintBackgroundEnabled( false );
    m_pTextLabel->SetVisible( false ); // By default hide.
    m_pTextLabel->SetWrap( true );
    m_pTextKvData = nullptr;
}

CZMRadialButton::~CZMRadialButton()
{
    delete[] m_pszCommand;
    delete m_pTextLabel;

    if ( m_pTextKvData )
        m_pTextKvData->deleteThis();
    m_pTextKvData = nullptr;
}

void CZMRadialButton::SetImage( IImage* image )
{
    m_pImage = image;

    if ( !m_pCurImage )
        m_pCurImage = image;
}

void CZMRadialButton::SetImageFocus( IImage* image )
{
    m_pImageFocus = image;
}

void CZMRadialButton::SetImageDisabled( IImage* image )
{
    m_pImageDisabled = image;
}

void CZMRadialButton::SetFocus( bool state )
{
    if ( m_bDisabled )
        return;


    if ( state )
    {
        m_pCurImage = m_pImageFocus;
    }
    else
    {
        m_pCurImage = m_pImage;
    }
}

void CZMRadialButton::SetDisabled( bool state )
{
    if ( state )
    {
        m_pCurImage = m_pImageDisabled;
    }
    else
    {
        m_pCurImage = m_pImage;
    }

    m_bDisabled = state;
}

void CZMRadialButton::SetCommand( const char* sz )
{
    int len = Q_strlen( sz ) + 1;
    delete[] m_pszCommand;
    m_pszCommand = new char[len];
    Q_strncpy( m_pszCommand, sz, len );
}

void CZMRadialButton::SetLabelData( KeyValues* kv )
{
    if ( m_pTextKvData )
        m_pTextKvData->deleteThis();

    if ( !kv )
    {
        m_pTextKvData = nullptr;
        return;
    }


    m_pTextKvData = kv->MakeCopy();

    ApplyLabelData();
}

void CZMRadialButton::ApplyLabelData()
{
    KeyValues* kv = m_pTextKvData;

    if ( !kv ) return;


    m_pTextLabel->SetWide( GetParent()->GetWide() * kv->GetFloat( "wide", 0.16f ) );
    m_pTextLabel->SetTall( GetParent()->GetTall() * kv->GetFloat( "tall", 0.16f ) );


    CSplitString vPos( kv->GetString( "pos", "" ), " " );

    if ( vPos.Count() > 0 )
    {
        int x = GetParent()->GetWide() * atof( vPos[0] );
        int y = 0;

        if ( vPos.Count() > 1 )
            y = GetParent()->GetTall() * atof( vPos[1] );

        m_pTextLabel->SetPos( x, y );
    }
    else
    {
        CenterLabelToButton();
    }

    m_pTextLabel->SetText( kv->GetString( "text" ) );
    m_pTextLabel->SetName( kv->GetString( "name" ) );


    if ( kv->GetBool( "center" ) )
        m_pTextLabel->SetContentAlignment( Label::Alignment::a_center );


    m_pTextLabel->SetCenterWrap( kv->GetBool( "centerwrap", false ) );
    m_pTextLabel->SetVisible( kv->GetBool( "visible", true ) );
    m_pTextLabel->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( kv->GetString( "font" ), IsProportional() ) );
    //m_pTextLabel->SetFgColor( kv->GetColor( "color" ) );
}

void CZMRadialButton::CenterLabelToButton()
{
    int size = GetParent()->GetWide() > GetParent()->GetTall() ? GetParent()->GetTall() : GetParent()->GetWide();
    int cx = size / 2;
    int cy = cx;

    float distcenter = ( GetStartFrac() + (GetEndFrac() - GetStartFrac()) / 2.0f ) * (float)cx;

    float rad = -DEG2RAD( AngleNormalize( GetOffset() + GetSize() / 2.0f ) );
    float ux = cos( rad );
    float uy = sin( rad );


    int x = cx + ux * distcenter;
    int y = cy + uy * distcenter;

    m_pTextLabel->SetPos( x - m_pTextLabel->GetWide() / 2, y - m_pTextLabel->GetTall() / 2 );
}

void CZMRadialButton::PerformLayout()
{
    Panel* pParent = GetParent();
    Assert( pParent );

    SetWide( pParent->GetWide() );
    SetTall( pParent->GetTall() );
}

void CZMRadialButton::Paint( int w, int h )
{
    if ( m_pCurImage )
    {
        m_pCurImage->SetPos( 0, 0 );
        m_pCurImage->SetSize( w, h );
        m_pCurImage->Paint();
    }
}


//DECLARE_BUILD_FACTORY( CZMRadialButton );
DECLARE_BUILD_FACTORY( CZMRadialPanel );


CZMRadialPanel::CZMRadialPanel( Panel* parent, const char* name ) : Panel( parent, name )
{
    SetKeyBoardInputEnabled( false );
    SetMouseInputEnabled( true );

    m_Buttons.Purge();
    m_pBgImage = nullptr;

    m_pLastButton = nullptr;
}

CZMRadialPanel::~CZMRadialPanel()
{
    m_Buttons.PurgeAndDeleteElements();
}

void CZMRadialPanel::SetBackgroundImage( const char* image )
{
    m_pBgImage = scheme()->GetImage( image, true );
}

void CZMRadialPanel::AddButton( KeyValues* kv )
{
    const char* command = kv->GetString( "command" );


    CZMRadialButton* btn = new CZMRadialButton( this, kv->GetName() );

    // Hardware = true if it's gonna be scaled.
    {
        IImage* pImage = scheme()->GetImage( kv->GetString( "img" ), true );
        if ( pImage && surface()->IsTextureIDValid( pImage->GetID() ) )
        {
            btn->SetImage( pImage );
        }
    }

    {
        IImage* pImage = scheme()->GetImage( kv->GetString( "img_over" ), true );
        if ( pImage && surface()->IsTextureIDValid( pImage->GetID() ) )
        {
            btn->SetImageFocus( pImage );
        }
    }

    {
        IImage* pImage = scheme()->GetImage( kv->GetString( "img_disabled" ), true );
        if ( pImage && surface()->IsTextureIDValid( pImage->GetID() ) )
        {
            btn->SetImageDisabled( pImage );
        }
    }

    btn->SetSize( kv->GetFloat( "size" ) );
    btn->SetOffset( kv->GetFloat( "offset" ) );
    btn->SetStartFrac( kv->GetFloat( "startfrac", 0.0f ) );
    btn->SetEndFrac( kv->GetFloat( "endfrac", 1.0f ) );

    if ( *command )
        btn->SetCommand( command );


    btn->SetLabelData( kv->FindKey( "label" ) );

    m_Buttons.AddToTail( btn );
}

void CZMRadialPanel::LoadFromFile( const char* file )
{
    KeyValues* kv = new KeyValues( "Radial" );
    kv->UsesEscapeSequences( true );

    if ( kv->LoadFromFile( filesystem, file, "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();


        while ( pKey )
        {
            AddButton( pKey );

            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load radial buttons from file (%s)!\n", file );
    }

    kv->deleteThis();
}

CZMRadialButton* CZMRadialPanel::GetButton( int x, int y )
{
    float r = ( GetWide() > GetTall() ? GetTall() : GetWide() ) / 2.0f;



    float difx = x - r;
    float dify = r - y;

    float distsqr = Square( difx ) + Square( dify );

    if ( distsqr > Square( r ) ) // Too far away from center.
    {
        return nullptr;
    }

    
    float sf, ef;
    float size;
    float start, end;
    
    float deg = RAD2DEG( atan2f( dify, difx ) );

    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
    {
        sf = Square( m_Buttons[i]->GetStartFrac() * r );
        ef = Square( m_Buttons[i]->GetEndFrac() * r );

        if ( sf > distsqr || ef < distsqr ) continue;

        size = m_Buttons[i]->GetSize();
        start = m_Buttons[i]->GetOffset();
        end = AngleNormalize( start + size );
        
        if ( deg > start || deg < end )
        {
            // Account for the wrapping.
            if ( abs( (end - start) - size ) > 5.0f )
            {
                end += 360.0f;

                if ( deg < start && deg < end )
                    return m_Buttons[i];
            }

            if ( deg > start && deg < end )
                return m_Buttons[i];
        }
    }

    return nullptr;
}

void CZMRadialPanel::ApplySettings( KeyValues* inResourceData )
{
    BaseClass::ApplySettings( inResourceData );
}

void CZMRadialPanel::GetSettings( KeyValues* outResourceData )
{
    BaseClass::GetSettings( outResourceData );
}

const char* CZMRadialPanel::GetDescription()
{
    //static char buf[1024];
    //_snprintf(buf, sizeof( buf ), "%s, string image, string border, string fillcolor, bool scaleImage", BaseClass::GetDescription());
    return BaseClass::GetDescription();
}

void CZMRadialPanel::OnMousePressed( MouseCode code )
{
    CZMRadialButton* button = m_pLastButton;

    if ( code == MOUSE_LEFT && button )
    {
        Panel* parent = GetParent();

        if ( parent && button && !button->IsDisabled() && button->GetCommand() && *button->GetCommand() )
        {
            parent->OnCommand( button->GetCommand() );
        }
    }
}

// OnMousePressed is actually never fired for double presses.
// The effect is "missing" clicks which is incredibly annoying.
void CZMRadialPanel::OnMouseDoublePressed( MouseCode code )
{
	OnMousePressed(code);
}

void CZMRadialPanel::OnThink()
{
    if ( !IsVisible() ) return;
    

    int mx, my;
    int myx, myy;
    int px = 0;
    int py = 0;

    Panel* parent = GetParent();
    if ( parent )
    {
        parent->GetPos( px, py );
    }

    ::input->GetFullscreenMousePos( &mx, &my );
    GetPos( myx, myy );

    CZMRadialButton* button = IsCursorOver() ? GetButton( mx - (px + myx), my - (py + myy) ) : nullptr;


    if ( m_pLastButton != button )
    {
        if ( m_pLastButton ) // Send mouse messages to parent.
        {
            KeyValues* kv = new KeyValues( "OnRadialLeave" );
            kv->SetPtr( "button", m_pLastButton );
            PostMessage( GetParent(), kv );
        }

        if ( button )
        {
            KeyValues* kv = new KeyValues( "OnRadialOver" );
            kv->SetPtr( "button", button );
            PostMessage( GetParent(), kv );
        }
    }

    // Update whether we should capture the mouse. Only capture while the mouse is on top our elements.
    if ( button )
    {
        if ( GetCursor() != dc_hand )
            SetCursor( dc_hand );

        if ( !IsMouseInputEnabled() )
            SetMouseInputEnabled( true );
    }
    else
    {
        if ( GetCursor() != dc_arrow )
            SetCursor( dc_arrow );

        if ( IsMouseInputEnabled() )
            SetMouseInputEnabled( false );
    }

    UpdateButtonFocus( button );


    m_pLastButton = button;
}

void CZMRadialPanel::UpdateButtonFocus( CZMRadialButton* button )
{
    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_Buttons[i]->SetFocus( ( m_Buttons[i] == button ) ? true : false );
    }
}

void CZMRadialPanel::PaintBackground()
{
    int w, h;
    GetSize( w, h );

    if ( m_pBgImage )
    {
        m_pBgImage->SetPos( 0, 0 );
        m_pBgImage->SetSize( w, h );
        m_pBgImage->Paint();
    }
}

void CZMRadialPanel::Paint()
{
    int w, h;
    GetSize( w, h );

    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
        m_Buttons[i]->Paint( w, h );
}
