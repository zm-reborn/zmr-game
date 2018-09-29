#pragma once


#include "zmr_mainmenu_basebtn.h"


class CZMMainMenuSubButton;

class CZMMainMenuButton : public CZMMainMenuBaseButton
{
public:
    typedef CZMMainMenuBaseButton BaseClass;
    typedef CZMMainMenuButton ThisClass;
    //DECLARE_CLASS( CZMMainMenuButton, CZMMainMenuBaseButton );


    CZMMainMenuButton( vgui::Panel* pParent, const char* name );
    ~CZMMainMenuButton();


    virtual void OnCursorEntered() OVERRIDE;
    virtual void OnCursorExited() OVERRIDE;
    virtual void OnMousePressed( vgui::MouseCode code ) OVERRIDE;

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;


    void AttemptToShowButtons();
    void ShowSubButtons();
    void HideSubButtons();


    void PositionSubButtons();

    bool IsSubButtonsVisible() const;
    bool DrawOnlyInGame() const { return m_bOnlyInGame; }

    void SetDrawOnlyInGame( bool state ) { m_bOnlyInGame = state; }

protected:
    void AddSubButton( const char* label, const char* command );

private:
    CUtlVector<CZMMainMenuSubButton*> m_vSubBtns;

    bool m_bOnlyInGame;
};
