#include "cbase.h"
#include "filesystem.h"
#include "input.h"

#include "zmr_radial.h"



using namespace vgui;


CZMRadialButton::CZMRadialButton( Panel* parent, const char* name ) : Panel( parent, name )
{
    m_pCurImage = nullptr;
    m_pImage = nullptr;
    m_pImageFocus = nullptr;
    m_pImageDisabled = nullptr;
    m_pszCommand = nullptr;

    m_pTextLabel = new Label( this, name, "WAT" );
}

CZMRadialButton::~CZMRadialButton()
{
    delete[] m_pszCommand;
    delete m_pTextLabel;
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


DECLARE_BUILD_FACTORY( ZMRadialPanel );


ZMRadialPanel::ZMRadialPanel( Panel *parent, const char *name ) : Panel( parent, name )
{
    m_Buttons.Purge();
    m_pBgImage = nullptr;
}

ZMRadialPanel::~ZMRadialPanel()
{
    m_Buttons.PurgeAndDeleteElements();
}

void ZMRadialPanel::SetBackgroundImage( const char* image )
{
    m_pBgImage = scheme()->GetImage( image, true );
}

void ZMRadialPanel::AddButton( const char* image, const char* imagefocus, float size, float offset, float start, float end, const char* command )
{
    IImage* pImage = nullptr;
    IImage* pImageFocus = nullptr;

    // Hardware = true if it's gonna be scaled.
    if ( image && *image )
        pImage = scheme()->GetImage( image, true );

    if ( imagefocus && *imagefocus )
        pImageFocus = scheme()->GetImage( imagefocus, true );


    CZMRadialButton* btn = new CZMRadialButton( this, "" );

    btn->SetImage( pImage );
    btn->SetImageFocus( pImageFocus );
    btn->SetSize( size );
    btn->SetOffset( offset );
    btn->SetStartFrac( start );
    btn->SetEndFrac( end );

    if ( command && *command )
        btn->SetCommand( command );

    m_Buttons.AddToTail( btn );
}

void ZMRadialPanel::LoadFromFile( const char* file )
{
    KeyValues* kv = new KeyValues( "Radial" );

    if ( kv->LoadFromFile( filesystem, file, "MOD" ) )
    {
        KeyValues* pKey = kv->GetFirstSubKey();


        while ( pKey )
        {
            AddButton(  pKey->GetString( "img" ),
                        pKey->GetString( "img_over" ),
                        pKey->GetFloat( "size" ),
                        pKey->GetFloat( "offset" ),
                        pKey->GetFloat( "startfrac", 0.0f ),
                        pKey->GetFloat( "endfrac", 1.0f ),
                        pKey->GetString( "command" ) );

            pKey = pKey->GetNextKey();
        }
    }
    else
    {
        Warning( "Couldn't load radial buttons from file (%s)!\n", file );
    }

    kv->deleteThis();
}

CZMRadialButton* ZMRadialPanel::GetButton( int x, int y )
{
    float r = GetWide() / 2.0f;



    float difx = x - r;
    float dify = r - y;

    float distsqr = Square( difx ) + Square( dify );

    if ( distsqr > Square( r ) )
    {
        return nullptr;
    }

    
    float sf, ef;
    float size;
    float start, end;
    
    float rad = RAD2DEG( atan2f( dify, difx ) );

    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
    {
        sf = Square( m_Buttons[i]->GetStartFrac() * r );
        ef = Square( m_Buttons[i]->GetEndFrac() * r );

        if ( sf > distsqr || ef < distsqr ) continue;

        size = m_Buttons[i]->GetSize();
        start = m_Buttons[i]->GetOffset();
        end = AngleNormalize( start + size );
        
        if ( rad > start || rad < end )
        {
            // Account for the wrapping.
            if ( abs( (end - start) - size ) > 5.0f )
            {
                end += 360.0f;

                if ( rad < start && rad < end )
                    return m_Buttons[i];
            }

            if ( rad > start && rad < end )
                return m_Buttons[i];
        }
    }

    return nullptr;
}

void ZMRadialPanel::ApplySettings( KeyValues* inResourceData )
{
    BaseClass::ApplySettings( inResourceData );
}

void ZMRadialPanel::GetSettings( KeyValues* outResourceData )
{
    BaseClass::GetSettings( outResourceData );
}

const char* ZMRadialPanel::GetDescription()
{
    //static char buf[1024];
    //_snprintf(buf, sizeof( buf ), "%s, string image, string border, string fillcolor, bool scaleImage", BaseClass::GetDescription());
    return BaseClass::GetDescription();
}

void ZMRadialPanel::OnMouseReleased( MouseCode code )
{
    if ( code == MOUSE_LEFT )
    {
        Panel* parent = GetParent();
        if ( !parent ) return;

        int mx, my;
        int px, py;
        int myx, myy;

        ::input->GetFullscreenMousePos( &mx, &my );
        parent->GetPos( px, py );
        GetPos( myx, myy );

        CZMRadialButton* button = GetButton( mx - (px + myx), my - (py + myy) );

        if ( button && button->GetCommand() && *button->GetCommand() )
        {
            Msg( "Pressed button: %s\n", button->GetCommand() );
            parent->OnCommand( button->GetCommand() );
        }
    }
}

void ZMRadialPanel::OnCursorMoved( int x, int y )
{
    CZMRadialButton* button = GetButton( x, y );

    if ( button )
    {
        if ( GetCursor() != dc_hand )
            SetCursor( dc_hand );
    }
    else
    {
        if ( GetCursor() != dc_arrow )
            SetCursor( dc_arrow );
    }

    UpdateButtonFocus( button );
}

void ZMRadialPanel::UpdateButtonFocus( CZMRadialButton* button )
{
    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
    {
        m_Buttons[i]->SetFocus( ( m_Buttons[i] == button ) ? true : false );
    }
}

void ZMRadialPanel::PaintBackground()
{
    int w, h;
    GetSize( w, h );


    if ( m_pBgImage )
    {
        m_pBgImage->SetPos( 0, 0 );
        m_pBgImage->SetSize( w, h );
        m_pBgImage->Paint();
    }

    int len = m_Buttons.Count();
    for ( int i = 0; i < len; i++ )
        m_Buttons[i]->Paint( w, h );
}
