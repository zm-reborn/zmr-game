#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <game/client/iviewport.h>

#include "viewport_panel_names.h"


class CZMHudSpectatorUI : public CHudElement, public vgui::EditablePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudSpectatorUI, vgui::EditablePanel );

    CZMHudSpectatorUI( const char* pElementName );


    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void LevelInit() OVERRIDE;
    virtual void Reset() OVERRIDE;

    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;

    virtual bool IsVisible() OVERRIDE;

protected:
    void PaintBar( int y, int h, bool bFlip );
    void PaintBorder( int border_y, int border_height, bool bFlip );

    void Update();
    bool UpdateTargetText();

private:
    int m_nTexPanelBgId;
    int m_nTexPanelBgTopId;

    C_BaseEntity*   m_pOldTarget;
    int             m_nOldTargetHealth;
    int             m_nOldObserverMode;

    CPanelAnimationVar( Color, m_BarColor, "BarColor", "0 0 0 220" );
    CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "ZMHudVoteText" );


    vgui::Label*    m_pNameLabel;
    vgui::Label*    m_pInfoLabel;
};

/*class CZMHudSpectatorUI : public vgui::EditablePanel, public IViewPortPanel
{
public:
	DECLARE_CLASS_SIMPLE( CZMHudSpectatorUI, vgui::EditablePanel );


    CZMHudSpectatorUI( IViewPort* pViewPort );
    ~CZMHudSpectatorUI();


	virtual const char*     GetName() OVERRIDE { return ""; };
	virtual void            SetData( KeyValues* data ) OVERRIDE {};
	virtual void            Reset() OVERRIDE {};
	virtual bool            NeedsUpdate() OVERRIDE { return false; };
	virtual bool            HasInputElements() OVERRIDE { return false; };
    vgui::VPANEL            GetVPanel() OVERRIDE { return BaseClass::GetVPanel(); };
	virtual bool            IsVisible() OVERRIDE { return BaseClass::IsVisible(); };
	virtual void            SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); };

    virtual void            ShowPanel( bool bShow ) OVERRIDE;
    virtual void            Update() OVERRIDE;


    virtual void    OnThink() OVERRIDE;
    virtual void    Paint() OVERRIDE;
    virtual void    PerformLayout() OVERRIDE;
    virtual void    ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

private:
    bool m_bSpecScoreboard;

    int m_nTexPanelBgId;


    //vgui::Panel* m_pBottomBarBlank;
    //vgui::Panel* m_pTopBar;
};*/
