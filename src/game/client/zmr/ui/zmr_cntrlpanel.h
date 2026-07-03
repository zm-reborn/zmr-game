#pragma once

//#include "baseviewport.h"

#include "zmr_bitmapbutton.h"


class CZMHudControlPanel : public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudControlPanel, vgui::Panel );

    CZMHudControlPanel( vgui::Panel* pParent );
    
    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;

    virtual void PerformLayout() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void OnCommand( const char* command ) OVERRIDE;
    
    void SetBgColor( const Color& clr );
    void SetFgColor( const Color& clr );
    virtual void PaintBackground() OVERRIDE;

    void PositionButtons();
    void PositionComboBox();


    void GroupsListUpdate();
    void CreateGroup();
    void SelectGroup();
    

    void UpdateTabs( int activatedTab = -1 );

    enum
    {
        BUTTON_POWER_PHYSEXP,
        BUTTON_POWER_NIGHTVISION,
        BUTTON_MODE_OFFENSIVE,
        BUTTON_MODE_DEFENSIVE,
        BUTTON_TOOL_SELECTALL,
        BUTTON_MODE_AMBUSH,
        BUTTON_GROUP_CREATE,
        BUTTON_GROUP_GOTO,
        BUTTON_POWER_SPOTCREATE,
        BUTTON_POWER_DELETEZOMBIES,
        BUTTON_MODE_CEILING,

        NUM_BUTTONS
    };

    enum
    {
        TAB_MODES,
        TAB_POWERS,
        TAB_ZEDS,

        NUM_TABS
    };
private:
    void CreateButtons();

    CZMBitMapButton* m_pButtons[NUM_BUTTONS];
    CZMBitMapButton* m_pTabs[NUM_TABS];

    int m_iActiveTab;

    Color m_BgColor;
    Color m_FgColor;

    CUtlVector<int> m_ComboBoxItems;
    vgui::ComboBox* m_pZombieGroups;

    int m_nTexBgId;

    static const int BUTTON_SIZE = 16;
    static const int BUTTON_SPACING = 4;
    static const int MARGIN_TOP = 12;
    static const int MARGIN_LEFT = 18;
    static const int MARGIN_BOTTOM_RIGHT = 8;

    static const int TAB_SPACING = 8;
};
