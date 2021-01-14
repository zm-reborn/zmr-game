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


    virtual void OnCursorExited() OVERRIDE;

    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;

    virtual void PerformLayout() OVERRIDE;

    virtual void DoClick() OVERRIDE;
    virtual void SetSelected( bool state ) OVERRIDE;
    virtual void SetArmed( bool state ) OVERRIDE;


    void AttemptToShowButtons();
    void ShowSubButtons();
    void HideSubButtons();


    void PositionSubButtons();


    int GetSubButtonHeight() const { return m_nSubBtnHeight; }
    int GetMaxSubTextWidth() const { return m_nMaxSubTextWidth; }
    float GetArmedTime() const { return m_flArmedTime; }
    float GetUnarmedTime() const { return m_flUnarmedTime; }
    int GetSubButtonCount() const { return m_vSubBtns.Count(); }

    bool IsLayoutHorizontally() const { return m_bLayoutHorizontally; }
    int GetHorizontalMargin() const { return m_nHorizontalMargin; }
    
    bool IsSubButtonsVisible() const;
    bool DrawOnlyInGame() const { return m_bOnlyInGame; }
    bool DrawOnlyNotInGame() const { return m_bOnlyNotInGame; }

    void SetDrawOnlyInGame( bool state ) { m_bOnlyInGame = state; }

protected:
    void AddSubButton( const char* label, const char* command );
    void AddSubButton( KeyValues* kv );

    void ComputeMaxTextWidth();

private:
    CUtlVector<CZMMainMenuSubButton*> m_vSubBtns;

    bool m_bOnlyInGame;
    bool m_bOnlyNotInGame;

    bool m_bLayoutHorizontally;
    int m_nHorizontalMargin;
    
    int m_nMaxSubTextWidth;
    int m_nSubBtnHeight;

    float m_flArmedTime;
    float m_flUnarmedTime;
};
