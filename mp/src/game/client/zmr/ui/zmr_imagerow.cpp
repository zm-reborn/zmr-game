#include "cbase.h"
#include "input.h"


#include "zmr_imagerow.h"


using namespace vgui;


DECLARE_BUILD_FACTORY( CZMImageRow );


CZMImageRow::CZMImageRow( Panel* parent, const char* name ) : Panel( parent, name )
{
    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( false );

    m_nImageSize = 0;
    m_pLayoutFunc = nullptr;
    m_iDirection = DIR_X;
}

CZMImageRow::~CZMImageRow()
{
    RemoveImages();
}

int CZMImageRow::AddImage( const char* image )
{
    IImage* pImage = scheme()->GetImage( image, true );

    if ( pImage )
    {
        return m_Images.AddToTail( new CZMPlaceImage( pImage ) );
    }

    return m_Images.InvalidIndex();
}

int CZMImageRow::AddImage( vgui::IImage* pImage )
{
    if ( pImage )
    {
        return m_Images.AddToTail( new CZMPlaceImage( pImage ) );
    }

    return m_Images.InvalidIndex();
}

bool CZMImageRow::SetImage( int index, vgui::IImage* pImage )
{
    if ( !m_Images.IsValidIndex( index ) )
        return false;

    m_Images[index]->SetImage( pImage );
    return true;
}

bool CZMImageRow::RemoveImageByIndex( int index )
{
    if ( !m_Images.IsValidIndex( index ) )
        return false;

    m_Images.Remove( index );
    return true;
}

void CZMImageRow::RemoveImages()
{
    m_Images.PurgeAndDeleteElements();
}

IImage* CZMImageRow::GetImageByIndex( int index )
{
    if ( !m_Images.IsValidIndex( index ) )
        return nullptr;

    return m_Images[index]->GetImage();
}

int CZMImageRow::GetImageIndexAtPos( int x, int y )
{
    int x1, y1, x2, y2;
    for ( int i = 0; i < m_Images.Count(); i++ )
    {
        m_Images[i]->GetPos( x1, y1 );
        m_Images[i]->GetSize( x2, y2 );
        x2 += x1;
        y2 += y1;

        if ( x >= x1 && y >= y1 && x <= x2 && y <= y2 )
            return i;
    }

    return -1;
}

void CZMImageRow::LayoutImages()
{
    if ( !m_Images.Count() ) return;

    if ( m_pLayoutFunc )
    {
        m_pLayoutFunc( this, &m_Images );
        return;
    }

    DefaultLayout();
}

void CZMImageRow::DefaultLayout()
{
    int size = 0;

    switch ( m_iDirection )
    {
    case DIR_Y : size = GetTall(); break;
    default : size = GetWide(); break;
    }

    Assert( size != 0 );


    int xDir = m_iDirection == DIR_X ? 1 : 0;
    int yDir = m_iDirection == DIR_Y ? 1 : 0;
    Assert( xDir != 0 || yDir != 0 );

    int imgsize = GetImagesSize();

    //float f = (imgsize * m_Images.Count()) / (float)size;


    int jump = imgsize;

    int x = 0;
    int y = 0;
    for ( int i = 0; i < m_Images.Count(); i++ )
    {
        if ( x > size )
        {
            x = 0;
            y += jump;
        }
        if ( y > size )
        {
            y = 0;
            x += jump;
        }

        m_Images[i]->SetPos( x, y );
        m_Images[i]->SetSize( imgsize, imgsize );
        x += xDir * jump;
        y += yDir * jump;
    }
}

void CZMImageRow::OnMousePressed( MouseCode code )
{
    if ( m_iLastImageOver != -1 )
    {
        KeyValues* kv = new KeyValues( "OnImageRowPressed" );
        kv->SetInt( "index", m_iLastImageOver );
        PostMessage( GetParent(), kv );
    }
}

void CZMImageRow::OnThink()
{
    if ( !IsVisible() ) return;


    // Update whether we should capture the mouse. Only capture while the mouse is on top our elements.
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
    

    m_iLastImageOver = GetImageIndexAtPos( mx - (px + myx), my - (py + myy) );

    if ( m_iLastImageOver != -1 )
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
}

void CZMImageRow::PerformLayout()
{
    LayoutImages();
}

void CZMImageRow::Paint()
{
    for ( int i = 0; i < m_Images.Count(); i++ )
    {
        m_Images[i]->Paint();
    }
}