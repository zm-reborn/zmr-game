#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Image.h>
#include <vgui/IScheme.h>


#include "zmr_shareddefs.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


class CZMMainMenuImagePanel : public vgui::ImagePanel
{
public:
    typedef vgui::ImagePanel BaseClass;
    typedef CZMMainMenuImagePanel ThisClass;
    //DECLARE_CLASS( CZMMainMenuImagePanel, vgui::ImagePanel );


    CZMMainMenuImagePanel( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuImagePanel();


    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;
    virtual void PerformLayout() OVERRIDE;

private:
    bool m_bAlignBottom;
    bool m_bGetBottom;
    int m_nOriginalHeight;
    int m_iBottomY;
};


DECLARE_BUILD_FACTORY( CZMMainMenuImagePanel );


using namespace vgui;


CZMMainMenuImagePanel::CZMMainMenuImagePanel( vgui::Panel* pParent, const char* name ) : BaseClass( pParent, name )
{
    m_bAlignBottom = false;
    m_bGetBottom = true;
    m_nOriginalHeight = 0;
    m_iBottomY = 0;
}

CZMMainMenuImagePanel::~CZMMainMenuImagePanel()
{
    
}

void CZMMainMenuImagePanel::ApplySettings( KeyValues* kv )
{
    m_bAlignBottom = kv->GetBool( "alignbottom", false );


    auto* images = kv->FindKey( "images" );

    if ( images )
    {
        struct ZMImgData_t
        {
            const char* pszImageName;
            int iEvent;
        };

        CUtlVector<ZMImgData_t> vImages;

        for ( auto* pImage = images->GetFirstTrueSubKey(); pImage; pImage = pImage->GetNextTrueSubKey() )
        {
            const char* imgname = pImage->GetString( "img" );
            int iEvent = pImage->GetInt( "event", HZEVENT_INVALID );

            if ( *imgname )
            {
                vImages.AddToTail( { imgname, iEvent } );
            }
        }
        
        if ( vImages.Count() )
        {
            ZMImgData_t* pImageData = nullptr;

            // ZMRTODO: This event thing doesn't work right now
            // because this cvar is handled only on the server.
            int iCurEvent = ConVarRef( "zm_sv_happyzombies" ).GetInt();

            do
            {
                int index = random->RandomInt( 0, vImages.Count() - 1 );
                ZMImgData_t& data = vImages[index];

                if ( data.iEvent > HZEVENT_INVALID && data.iEvent != iCurEvent )
                {
                    vImages.Remove( index );
                    continue;
                }


                pImageData = &vImages.Element( index );
                break;
            }
            while ( vImages.Count() > 0 );

            
            if ( pImageData )
            {
                SetImage( pImageData->pszImageName );
            }
        }
    }

    BaseClass::ApplySettings( kv );
}

void CZMMainMenuImagePanel::PerformLayout()
{
    BaseClass::PerformLayout();

    int cw, ch;
    GetSize( cw, ch );


    if ( m_bGetBottom )
    {
        m_bGetBottom = false;
        m_nOriginalHeight = ch;
        m_iBottomY = GetYPos() + m_nOriginalHeight;
    }


    // Change the image height so the image doesn't look stretched.
    auto* pImage = GetImage();
    if ( pImage )
    {
        int imgw, imgh;
        pImage->GetContentSize( imgw, imgh );



        float intendedratio = 1024 / (float)512;

        float imgratio = imgh / (float)imgw;

        float curratio = ch / (float)cw;

        if ( fabs( curratio - imgratio ) > 0.2f )
        {
            float frac = intendedratio / imgratio;

            ch = m_nOriginalHeight / frac;
            SetTall( ch );
        }
    }


    // Align the image using the bottom corner
    if ( m_bAlignBottom )
    {
        SetPos( GetXPos(), m_iBottomY - ch );
    }
}
