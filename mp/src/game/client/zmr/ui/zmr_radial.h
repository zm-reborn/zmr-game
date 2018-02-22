#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/Label.h>


using namespace vgui;

class CZMRadialButton : public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMRadialButton, Panel );


    CZMRadialButton( Panel* parent, const char* name );
    ~CZMRadialButton();


    inline void SetImage( IImage* image ) { m_pImage = image; if ( !m_pCurImage ) m_pCurImage = image; };
    inline void SetImageFocus( IImage* image ) { m_pImageFocus = image; if ( !m_pCurImage ) m_pCurImage = image; };

    void SetFocus( bool state )
    {
        if ( state )
        {
            m_pCurImage = m_pImageFocus;
        }
        else
        {
            m_pCurImage = m_pImage;
        }
    }

    inline const char* GetCommand() { return m_pszCommand; };
    void SetCommand( const char* sz )
    {
        int len = Q_strlen( sz ) + 1;
        delete[] m_pszCommand;
        m_pszCommand = new char[len];
        Q_strncpy( m_pszCommand, sz, len );
    }

    inline float GetSize() { return m_flSize; };
    inline void SetSize( float size ) { m_flSize = AngleNormalize( size ); };
    inline float GetOffset() { return m_flOffset; };
    inline void SetOffset( float offset ) { m_flOffset = AngleNormalize( offset ); };
    inline float GetStartFrac() { return m_flStartFrac; };
    inline void SetStartFrac( float frac ) { m_flStartFrac = frac; };
    inline float GetEndFrac() { return m_flEndFrac; };
    inline void SetEndFrac( float frac ) { m_flEndFrac = frac; };

    inline void SetLabelText( const char* txt ) { m_pTextLabel->SetText( txt ); };

    void Paint( int w, int h );

private:
    float m_flOffset;
    float m_flStartFrac;
    float m_flEndFrac;
    float m_flSize;

    char* m_pszCommand;

    IImage* m_pCurImage;

    IImage* m_pImage;
    IImage* m_pImageFocus;
    IImage* m_pImageDisabled;
    Label* m_pTextLabel;
};

class ZMRadialPanel : public Panel
{
public:
    DECLARE_CLASS_SIMPLE( ZMRadialPanel, Panel );

    ZMRadialPanel( Panel *parent, const char *name );
    ~ZMRadialPanel();


    void AddButton( const char* image, const char* imagefocus, float size, float offset, float start = 0.0f, float end = 1.0f, const char* command = "" );
    void LoadFromFile( const char* file );

    void SetBackgroundImage( const char* );

    
    virtual void OnMouseReleased( MouseCode ) OVERRIDE;
    virtual void OnCursorMoved( int, int ) OVERRIDE;
    virtual void ApplySettings( KeyValues* inResourceData ) OVERRIDE;

protected:
    virtual void GetSettings( KeyValues* outResourceData ) OVERRIDE;
    virtual const char* GetDescription() OVERRIDE;
    
    virtual void PaintBackground() OVERRIDE;
    //virtual void OnSizeChanged(int newWide, int newTall);
    //virtual void ApplySchemeSettings( IScheme *pScheme );

private:
    CZMRadialButton* GetButton( int x, int y );
    void UpdateButtonFocus( CZMRadialButton* );

    CUtlVector<CZMRadialButton*> m_Buttons;
    IImage* m_pBgImage;
};
