#pragma once

//#include "baseviewport.h"

#include "zmr_bitmapbutton.h"


class CZMHudControlPanel// : public CHudElement, public vgui::Frame
{
public:
    //DECLARE_CLASS_SIMPLE( CZMHudControlPanel, vgui::Frame );

    CZMHudControlPanel();
    //CZMHudControlPanel( const char* );
    ~CZMHudControlPanel();


    
    //virtual void Init() OVERRIDE;
    //virtual void VidInit() OVERRIDE;
    //virtual void Paint() OVERRIDE;
    //virtual void OnCommand( const char* command ) OVERRIDE;
    //virtual void OnThink() OVERRIDE;
    //virtual void Reset() OVERRIDE;
    
    void SetBgColor( const Color& clr );
    void Paint();

    void PositionButtons();
    void PositionComboBox();


    void GroupsListUpdate();
    void RemoveGroup( int serial );
    

    void UpdateTabs( int activatedTab = -1 );
    

    //qck: Keeps track of ents by handle serial numbers. They are unique, so there shouldn't be any problem.
    CUtlVector<int> m_ComboBoxItems;
    vgui::ComboBox *m_pZombieGroups;

    int m_nTexBgId;




    enum// Button
    { 
        BUTTON_POWER_PHYSEXP,
        BUTTON_POWER_NIGHTVISION,
        BUTTON_MODE_OFFENSIVE,
        BUTTON_MODE_DEFENSIVE,
        BUTTON_TOOL_SELECTALL,
        BUTTON_MODE_AMBUSH,
        BUTTON_GROUP_CREATE,
        BUTTON_GROUP_GOTO,
        BUTTON_POWER_SPOTCREATE, // LAWYER: Spot create stuff
        BUTTON_POWER_DELETEZOMBIES, // LAWYER: Spot create stuff
        BUTTON_MODE_CEILING, //fastie ceiling ambush
        NUM_BUTTONS	//needs to be last! holds number of icons in enum
    };

    enum// Tab
    {
        TAB_MODES,
        TAB_POWERS,
        TAB_ZEDS,
        NUM_TABS
    };



private:

    //create buttons
    void LoadButtons();
    //remove buttons
    void RemoveButtons();

    CZMBitMapButton *m_pButtons[NUM_BUTTONS];
    CZMBitMapButton *m_pTabs[NUM_TABS];

    int m_iActiveTab;

    Color m_BgColor;

    //see CBaseZombieMasterViewPort constructor on why these are needed -> the SetBounds part
    static const int HOR_ADJUST = 8; //8
    static const int VER_ADJUST = 40; //28
    //base positioning values
    static const int BUTTON_SIZE = 32; //32 //40
    static const int BUTTON_SPACING = 10;
    static const int PANEL_SPACING = 5;

    //TGB: the constants here assumed a 4:3 resolution
    //TGB: to fix we can simply define desired panel size here, and use that in combo with ScreenWidth to place on right
    //static const int PANEL_TOPLEFT_X = 540 + HOR_ADJUST - PANEL_SPACING;
    //static const int PANEL_TOPLEFT_Y = 380 + VER_ADJUST - PANEL_SPACING;
    //static const int PANEL_BOTRIGHT_X = 640 + HOR_ADJUST - PANEL_SPACING;
    //static const int PANEL_BOTRIGHT_Y = 480 + VER_ADJUST - PANEL_SPACING;
    
    static const int PANEL_SIZE_X = 156; // 156
    static const int PANEL_SIZE_Y = 150; // 156

    static const int COMBO_BOX_X_OFFSET = 11;
    static const int COMBO_BOX_Y_OFFSET = 64;
    static const int COMBO_BOX_WIDTH = 100;
};
