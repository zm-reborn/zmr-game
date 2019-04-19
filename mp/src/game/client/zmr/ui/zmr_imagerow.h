#pragma once

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>

#include "zmr_placeimg.h"
#include "zmr_radial.h"


class CZMImageRowItem;

typedef void (*ZMImageRowLayoutFunc_t)( vgui::Panel* pPanel, CUtlVector<CZMImageRowItem*>* pImages );
typedef CZMImageRowItem* (*ZMImageRowItemCreateFunc_t)( vgui::Panel* pParent );


//
class CZMImageRowItem : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMImageRowItem, vgui::Panel );

    
    CZMImageRowItem( vgui::Panel* pParent, const char* name );
    ~CZMImageRowItem();


    virtual void Paint() OVERRIDE;


    vgui::IImage* GetImage() const;
    CZMPlaceImage* GetPlaceImage() const;
    void SetImage( vgui::IImage* pImage );

    vgui::Label* GetLabel() const { return m_pLabel; }


    void SetText( const char* txt );

protected:
    vgui::Label* m_pLabel;
    CZMPlaceImage* m_pImage;
};
//


//
// Places images inside the panel with custom layout function.
class CZMImageRow : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMImageRow, vgui::Panel );

    CZMImageRow( vgui::Panel* pParent, const char* name );
    ~CZMImageRow();


    CZMImageRowItem* CreateItem();

    int     AddImage( const char* image );
    int     AddImage( vgui::IImage* pImage );
    bool    SetImage( int index, vgui::IImage* pImage );
    bool    SetText( int index, const char* txt );
    bool    RemoveImageByIndex( int index );
    void    RemoveImages();


    CZMImageRowItem*    GetItemByIndex( int index ) const;
    vgui::IImage*   GetImageByIndex( int index ) const;
    int             GetImageCount() const { return m_Images.Count(); }
    int             GetImageIndexAtPos( int x, int y );

    int     GetImagesSize() const { return m_nImageSize; }
    void    SetImagesSize( int size ) { m_nImageSize = size; }

    void    SetLayoutFunc( ZMImageRowLayoutFunc_t func ) { m_pLayoutFunc = func; }
    void    SetCreateFunc( ZMImageRowItemCreateFunc_t func ) { m_pCreateFunc = func; }

    void LayoutImages();
    void DefaultLayout();
    void SetImageSize( CZMImageRowItem* pItem );

    enum ImageLayoutDir
    {
        DIR_X,
        DIR_Y
    };


    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void PerformLayout() OVERRIDE;

private:
    CUtlVector<CZMImageRowItem*> m_Images;
    int m_iDirection;
    int m_nImageSize; // Wide and Height
    ZMImageRowLayoutFunc_t m_pLayoutFunc;
    ZMImageRowItemCreateFunc_t m_pCreateFunc;

    int m_iLastImageOver;
};
//
