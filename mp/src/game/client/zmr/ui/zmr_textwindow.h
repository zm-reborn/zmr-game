#pragma once


#include "vguitextwindow.h"


class CZMTextWindow : public CTextWindow
{
private:
    DECLARE_CLASS_SIMPLE( CZMTextWindow, CTextWindow );

public:
    CZMTextWindow( IViewPort* pViewPort );
    virtual ~CZMTextWindow();

    virtual void Update();
    virtual void SetVisible( bool state );
    virtual void ShowPanel( bool bShow );
    virtual void OnKeyCodePressed( vgui::KeyCode code );

protected:
    ButtonCode_t m_iScoreBoardKey;


public:
    virtual void PaintBackground();
    virtual void PerformLayout();
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
    bool m_backgroundLayoutFinished;
};
