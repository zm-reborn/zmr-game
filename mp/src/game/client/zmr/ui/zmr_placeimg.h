#pragma once

#include <vgui/IImage.h>

// Wraps IImage so you can draw the same image multiple times in different places, sizes, etc.
class CZMPlaceImage
{
public:
    CZMPlaceImage( vgui::IImage* pImage )
    {
        m_pImage = pImage; Assert( m_pImage );
        SetPos( 0, 0 );
        int w, h; pImage->GetSize( w, h ); SetSize( w, h );
    }

    inline void GetPos( int& x, int& y ) { x = m_iX; y = m_iY; };
    inline void SetPos( int x, int y ) { m_iX = x; m_iY = y; };
    inline void GetSize( int& w, int& h ) { w = m_iWide; h = m_iTall; };
    inline void SetSize( int w, int h ) { m_iWide = w; m_iTall = h; };

    inline vgui::IImage*    GetImage() { return m_pImage; };
    inline void             SetImage( vgui::IImage* pImage ) { m_pImage = pImage; Assert( m_pImage ); };

    
    inline void Paint() { m_pImage->SetPos( m_iX, m_iY ); m_pImage->SetSize( m_iWide, m_iTall ); m_pImage->Paint(); };

private:
    vgui::IImage* m_pImage;
    int m_iX, m_iY;
    int m_iWide, m_iTall;
};
