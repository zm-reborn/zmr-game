#include "cbase.h"


#include <vgui/ISurface.h>

#include "zmr_mainmenu_basebtn.h"


#include "zmr_mainmenu.h"
#include "zmr_mainmenu_btn.h"
#include "zmr_mainmenu_subbtn.h"


using namespace vgui;



CZMMainMenuBaseButton::CZMMainMenuBaseButton( Panel* pParent, const char* name ) : BaseClass( pParent, name, "" )
{
    SetPaintBackgroundEnabled( false );
    SetPaintBorderEnabled( false );


    m_pImage = nullptr;
    m_iImageX = 0;
    m_iImageY = 0;
    m_nImageMargin = 0;
}

CZMMainMenuBaseButton::~CZMMainMenuBaseButton()
{
}

void CZMMainMenuBaseButton::Paint()
{
    // Paint the image
    if ( m_pImage )
    {
        const Color clr = GetButtonFgColor();
        const int image_size = GetImageSize();

        m_pImage->SetColor( clr );
        m_pImage->SetPos( m_iImageX, m_iImageY );
        m_pImage->SetSize( image_size, image_size );
        m_pImage->Paint();
    }


    BaseClass::Paint();
}

void CZMMainMenuBaseButton::GetContentSize( int& wide, int& tall )
{
    BaseClass::GetContentSize( wide, tall );

    wide += m_pImage ? GetImageSize() : 0;
}

void CZMMainMenuBaseButton::PerformLayout()
{
    BaseClass::PerformLayout();

    LayoutImage();
}

void CZMMainMenuBaseButton::ApplySettings( KeyValues* in )
{
    BaseClass::ApplySettings( in );


    // Load image
    const char* image = in->GetString( "imagematerial" );

    if ( *image )
    {
        IImage* pImage = scheme()->GetImage( image, true );
        if ( pImage && surface()->IsTextureIDValid( pImage->GetID() ) )
        {
            m_pImage = pImage;
        }
    }


    m_nImageMargin = in->GetInt( "image_margin", 4 );

    // We have to layout here already to
    // account for auto-wide changes...
    LayoutImage();
}

int CZMMainMenuBaseButton::GetImageSize()
{
    int h = GetTall();
    int image_size = h * 0.5f;
    image_size += (image_size % 2) ? 1 : 0; // Make sure our size is even

    return image_size;
}

void CZMMainMenuBaseButton::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );



    LayoutImage();

    Color empty( 0, 0, 0, 0 );
    Color disabled( 64, 64, 64, 255 );
    Color armed( 255, 255, 200, 255 );

    SetDefaultColor( COLOR_WHITE, empty );
    SetArmedColor( armed, empty );
    SetDisabledFgColor1( disabled );
    SetDisabledFgColor2( empty );
    SetSelectedColor( armed, empty );
}

void CZMMainMenuBaseButton::LayoutImage()
{
    if ( m_pImage == nullptr )
        return;

    //
    // Layout the image and text
    //

    int image_size = GetImageSize();
    int image_y = GetTall() / 2 - image_size / 2;

    

    int tx0, ty0, tx1, ty1;
    ComputeAlignment( tx0, ty0, tx1, ty1 );


    int image_x = tx0 - image_size - m_nImageMargin;

    if ( image_x < 0 )
    {
        // Move the text to the right to leave room for the image.
        int ix, iy;
        GetTextInset( &ix, &iy );
        SetTextInset( -image_x, iy );

        image_x = 0;
    }
            

    m_iImageX = image_x;
    m_iImageY = image_y;
}

CZMMainMenu* CZMMainMenuBaseButton::GetMainMenu()
{
    return static_cast<CZMMainMenu*>( GetParent() );
}
