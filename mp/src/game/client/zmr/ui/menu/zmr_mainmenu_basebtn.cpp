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

        m_pImage->SetColor( clr );
        m_pImage->SetPos( m_iImageX, m_iImageY );
        m_pImage->SetSize( m_iImageSize, m_iImageSize );
        m_pImage->Paint();
    }


    BaseClass::Paint();
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


    // Layout the image
    
    int h = GetTall();
    int image_size = h * 0.5f;
    image_size += (image_size % 2) ? 1 : 0; // Make sure our size is even


    int image_y = h / 2 - image_size / 2;

    

    int tx0, ty0, tx1, ty1;
    ComputeAlignment( tx0, ty0, tx1, ty1 );


    int image_x = tx0 - image_size - 4;

    if ( image_x < 0 )
    {
        int ix, iy;
        GetTextInset( &ix, &iy );
        SetTextInset( ix + -1 * image_x, iy );
        image_x = 0;
    }
            

    m_iImageX = image_x;
    m_iImageY = image_y;
    m_iImageSize = image_size;
}

CZMMainMenu* CZMMainMenuBaseButton::GetMainMenu()
{
    return static_cast<CZMMainMenu*>( GetParent() );
}
