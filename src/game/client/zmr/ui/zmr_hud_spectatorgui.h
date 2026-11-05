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


    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) override;

    virtual void Init() override;
    virtual void VidInit() override;
    virtual void LevelInit() override;
    virtual void Reset() override;

    virtual void OnThink() override;
    virtual void Paint() override;

    virtual bool ShouldDraw() override;
    virtual bool IsVisible() override;

protected:
    void PaintBar( int y, int h, bool bFlip );
    void PaintBorder( int border_y, int border_height, bool bFlip );

    void Update();
    bool UpdateTargetText();

private:
    int m_nTexPanelBgId;
    int m_nTexPanelBgTopId;

    C_BaseEntity*   m_pOldTarget;
    int             m_iOldEntIndex;
    int             m_nOldTargetHealth;
    int             m_nOldObserverMode;
    int             m_iOldTeam;

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


	virtual const char*     GetName() override { return ""; };
	virtual void            SetData( KeyValues* data ) override {};
	virtual void            Reset() override {};
	virtual bool            NeedsUpdate() override { return false; };
	virtual bool            HasInputElements() override { return false; };
    vgui::VPANEL            GetVPanel() override { return BaseClass::GetVPanel(); };
	virtual bool            IsVisible() override { return BaseClass::IsVisible(); };
	virtual void            SetParent( vgui::VPANEL parent ) override { BaseClass::SetParent( parent ); };

    virtual void            ShowPanel( bool bShow ) override;
    virtual void            Update() override;


    virtual void    OnThink() override;
    virtual void    Paint() override;
    virtual void    PerformLayout() override;
    virtual void    ApplySchemeSettings( vgui::IScheme* pScheme ) override;

private:
    bool m_bSpecScoreboard;

    int m_nTexPanelBgId;


    //vgui::Panel* m_pBottomBarBlank;
    //vgui::Panel* m_pTopBar;
};*/
