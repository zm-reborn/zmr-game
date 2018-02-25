#pragma once

#include <vgui_controls/Panel.h>

#include "zmr_placeimg.h"
#include "zmr_radial.h"


typedef void (*ZMImageRowLayoutFunc_t)( vgui::Panel* pPanel, CUtlVector<CZMPlaceImage*>* pImages );

// Places images inside the panel with custom layout function.
class CZMImageRow : public vgui::Panel
{
public:
    CZMImageRow( vgui::Panel* parent, const char* name );
    ~CZMImageRow();


    int     AddImage( const char* image );
    int     AddImage( vgui::IImage* pImage );
    bool    SetImage( int index, vgui::IImage* pImage );
    bool    RemoveImageByIndex( int index );
    void    RemoveImages();

    vgui::IImage*   GetImageByIndex( int index );
    int             GetImageCount() { return m_Images.Count(); };
    int             GetImageIndexAtPos( int x, int y );

    int     GetImagesSize() { return m_nImageSize; };
    void    SetImagesSize( int size ) { m_nImageSize = size; };

    void    SetLayoutFunc( ZMImageRowLayoutFunc_t func ) { m_pLayoutFunc = func; };

    void LayoutImages();
    void DefaultLayout();

    enum ImageLayoutDir
    {
        DIR_X,
        DIR_Y
    };


    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void PerformLayout() OVERRIDE;
    virtual void Paint() OVERRIDE;

private:
    CUtlVector<CZMPlaceImage*> m_Images;
    int m_iDirection;
    int m_nImageSize; // Wide and Height
    ZMImageRowLayoutFunc_t m_pLayoutFunc;

    int m_iLastImageOver;
};