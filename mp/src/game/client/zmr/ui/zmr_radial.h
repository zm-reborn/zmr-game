#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/Label.h>


class CZMRadialButton : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMRadialButton, vgui::Panel );


    CZMRadialButton( vgui::Panel* parent, const char* name );
    ~CZMRadialButton();


    inline void SetImage( vgui::IImage* image );
    inline void SetImageFocus( vgui::IImage* image );
    inline void SetImageDisabled( vgui::IImage* image );

    void SetFocus( bool state );

    void SetDisabled( bool state );

    inline const char* GetCommand() { return m_pszCommand; };
    void SetCommand( const char* sz );

    inline float GetSize() { return m_flSize; };
    inline void SetSize( float size ) { m_flSize = AngleNormalize( size ); };
    inline float GetOffset() { return m_flOffset; };
    inline void SetOffset( float offset ) { m_flOffset = AngleNormalize( offset ); };
    inline float GetStartFrac() { return m_flStartFrac; };
    inline void SetStartFrac( float frac ) { m_flStartFrac = frac; };
    inline float GetEndFrac() { return m_flEndFrac; };
    inline void SetEndFrac( float frac ) { m_flEndFrac = frac; };

    inline vgui::Label* GetLabel() { return m_pTextLabel; };
    inline void SetLabelText( const char* txt ) { m_pTextLabel->SetText( txt ); };
    KeyValues* GetLabelData() { return m_pTextKvData; };
    void SetLabelData( KeyValues* kv );
    void ApplyLabelData();

    virtual void PerformLayout() OVERRIDE;
    void Paint( int w, int h );

private:
    void CenterLabelToButton();

    float m_flOffset;
    float m_flStartFrac;
    float m_flEndFrac;
    float m_flSize;

    char* m_pszCommand;

    // NOTE: Images are relative to materials/vgui
    vgui::IImage* m_pCurImage;

    vgui::IImage* m_pImage;
    vgui::IImage* m_pImageFocus;
    vgui::IImage* m_pImageDisabled;
    vgui::Label* m_pTextLabel;
    KeyValues* m_pTextKvData;
    bool m_bDisabled;
};

class CZMRadialPanel : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMRadialPanel, vgui::Panel );

    CZMRadialPanel( vgui::Panel* parent, const char* name );
    ~CZMRadialPanel();


    void                            AddButton( KeyValues* kv );
    void                            LoadFromFile( const char* file );
    CUtlVector<CZMRadialButton*>*   GetButtons() { return &m_Buttons; };

    void SetBackgroundImage( const char* image );

    
    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void ApplySettings( KeyValues* inResourceData ) OVERRIDE;

protected:
    virtual void GetSettings( KeyValues* outResourceData ) OVERRIDE;
    virtual const char* GetDescription() OVERRIDE;
    
    virtual void PaintBackground() OVERRIDE;
    virtual void Paint() OVERRIDE;

private:
    CZMRadialButton* GetButton( int x, int y );
    void UpdateButtonFocus( CZMRadialButton* button );

    CUtlVector<CZMRadialButton*> m_Buttons;
    vgui::IImage* m_pBgImage;

    CZMRadialButton* m_pLastButton;
};
