#include "cbase.h"

#include <vgui_controls/ComboBox.h>
#include "spectatorgui.h"
#include "iclientmode.h"
#include "vgui_bitmapbutton.h"


#include "zmr_global_shared.h"
#include "zmr_viewport.h"
#include "zmr_cntrlpanel.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;





//using namespace CZMHudControlPanel;
const int buttonToTab[CZMHudControlPanel::NUM_BUTTONS] = {
    CZMHudControlPanel::TAB_POWERS, //physexp
    CZMHudControlPanel::TAB_POWERS, //night vision
    CZMHudControlPanel::TAB_MODES, //defense
    CZMHudControlPanel::TAB_MODES, //offense,
    CZMHudControlPanel::TAB_MODES, //select all
    CZMHudControlPanel::TAB_MODES, //ambush
    CZMHudControlPanel::TAB_ZEDS, //create group
    CZMHudControlPanel::TAB_ZEDS, //goto group
    CZMHudControlPanel::TAB_POWERS, //spotcreate
    CZMHudControlPanel::TAB_POWERS, //delete
    CZMHudControlPanel::TAB_MODES //ceiling ambush
};

//DECLARE_HUDELEMENT( CZMHudControlPanel );

//CZMHudControlPanel::CZMHudControlPanel( const char* pElementName ) : CHudElement( pElementName ), Frame( g_pClientMode-CZMHudControlPanel::CZMHudControlPanel( const char* pElementName ) : CHudElement( pElementName ), Frame( g_pClientMode->GetViewport(), "ZMHudControlPanel", false )
CZMHudControlPanel::CZMHudControlPanel()
{
    /*
    SetMouseInputEnabled( false );
    SetKeyBoardInputEnabled( false );
    SetProportional( false );
    SetPaintBackgroundEnabled( false );
    SetMoveable( false );
    SetSizeable( false );

    SetScheme( scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" ) );
    LoadControlSettings( "resource/ui/zmcntrlpanel.res" );

    // Makes sure we don't get pressed / into a z-fight if other menus are on top of us.
    SetZPos( -100 );
    */
    LoadButtons();

    //Reset();


    UpdateTabs( CZMHudControlPanel::TAB_MODES );


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hud_bg_zmcntrl", true, false );
}

CZMHudControlPanel::~CZMHudControlPanel()
{
    RemoveButtons();
}
/*
void CZMHudControlPanel::Init()
{
    Reset();
}

void CZMHudControlPanel::VidInit()
{
    Reset();
}

void CZMHudControlPanel::Reset()
{
    SetPos( ScreenWidth() - PANEL_SIZE_X, ScreenHeight() - PANEL_SIZE_Y );

    if ( g_pZMView )
    {
        SetParent( g_pZMView );
    }
    
}

void CZMHudControlPanel::OnThink()
{
	if ( !IsVisible() ) return;


    if ( !IsCursorOver() ) return;


    // Make sure we have focus.
    MoveToFront();
}
*/

void CZMHudControlPanel::LoadButtons()
{
    const color32 white = { 255, 255, 255, 255 };
    const color32 grey = { 128, 128, 128, 255 };
    const color32 red = { 200, 55, 55, 255 };
    //-------
    //BUTTONS
    //-------

    // TODO: could mostly be moved into keyvalue files...

    //order needs to correspond with button enum
    const char *buttonMat[NUM_BUTTONS] = 
    {
        "VGUI/minishockwave",					//physexp
        "VGUI/minieye",							//night vision
        "VGUI/minicrosshair",					//offensive mode
        "VGUI/minishield",						//defensive mode
        "VGUI/miniselectall",					//select all
        "VGUI/miniarrows",						//ambush mode
        "VGUI/minigroupadd",					//create group
        "VGUI/minigroupselect",					//select group
        "VGUI/minispotcreate",					//spot create
        "VGUI/minideletezombies",				//Delete Zombies
        "VGUI/miniceiling",						//banshee ceiling jump/ambush
    };

    const char *buttonCmd[NUM_BUTTONS] =
    {
        "MODE_POWER_PHYSEXP",
        "MODE_POWER_NIGHTVISION",
        "MODE_OFFENSIVE",
        "MODE_DEFENSIVE",
        "MODE_SELECT_ALL",
        "MODE_AMBUSH_CREATE",
        "MODE_CREATE_GROUP",
        "MODE_SELECT_GROUP",
        "MODE_POWER_SPOTCREATE", //spot create
        "MODE_POWER_DELETEZOMBIES",
        "MODE_JUMP_CEILING",
    };

    //TGB: power costs are now printf'd into these, see toolTipCosts array below
    const char *toolTip[NUM_BUTTONS] =
    {
        "zmmenu_exp",
        "zmmenu_nv",
        "zmmenu_attack",
        "zmmenu_defend",
        "zmmenu_selectall",
        "zmmenu_ambush",
        "zmmenu_creategroup",
        "zmmenu_selectgroup",
        "zmmenu_createhidden",
        "zmmenu_delete",
        "zmmenu_bansheeceil"
    };

    const bool enabled[NUM_BUTTONS] =
    {
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true
    };

    /*const int toolTipCosts[NUM_BUTTONS] =
    {
        0,//zm_physexp_cost.GetInt(),	//explosion
        0,							//nightvis
        0,							//attack mode
        0,							//defend mode
        0,							//select all
        0,							//ambush mode
        0,							//squad create
        0,							//squad select
        0,//zm_spotcreate_cost.GetInt(),//hidden summon
        0,							//expire
        0,							//ceiling ambush
    };*/

    DevMsg("CZMHudControlPanel: creating buttons.\n");

    //load buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        m_pButtons[i] = new CZMBitMapButton( g_pZMView, buttonCmd[i], "" );
        m_pButtons[i]->SetProportional( false );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED, buttonMat[i], white );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, buttonMat[i], red );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_PRESSED, buttonMat[i], grey );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_DISABLED, buttonMat[i], grey );
        m_pButtons[i]->SetButtonBorderEnabled( false );
        m_pButtons[i]->SetPaintBorderEnabled( false );

        //basic command stuff
        m_pButtons[i]->SetCommand( buttonCmd[i] );

        m_pButtons[i]->SetTooltipName( toolTip[i] );

        m_pButtons[i]->SetEnabled( enabled[i] );
        
        //KeyValues *msg = new KeyValues("ButtonCommand");
        //msg->SetString("command", buttonCmd[i]);
        //m_pButtons[i]->SetCommand( msg );
        //m_pButtons[i]->AddActionSignalTarget( pParent );
        
        
        //m_pButtons[i]->m_bHasTooltip = true;

        //TGB: string juggling ahoy!
        //char buffer[256];
        //Q_snprintf(buffer, sizeof(buffer), toolTip[i], toolTipCosts[i]);
        //m_pButtons[i]->SetTooltip( nullptr, buffer );
        ///m_pButtons[i]->m_szMouseOverText = new char[sizeof(buffer)];
        //Q_strcpy(m_pButtons[i]->m_szMouseOverText, buffer);
        
    }

    //-------
    //TABS
    //-------
    const char *tabMat[NUM_TABS] = 
    {
        //temp mats
        "VGUI/minicrosshair",					//offensive mode
        "VGUI/minishockwave",					//physexp
        "VGUI/minigroupadd",					//zombie groups
    };

    const char *tabCmd[NUM_TABS] =
    {
        "TAB_MODES",
        "TAB_POWERS",
        "TAB_ZEDS",
    };

    const bool tabEnabled[NUM_BUTTONS] =
    {
        true,
        true,
        true
    };

    //load tab buttons
    for (int i = 0; i < NUM_TABS; i++)
    {
        m_pTabs[i] = new CZMBitMapButton( g_pZMView, tabCmd[i], "" );
        m_pTabs[i]->SetProportional( false );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED, tabMat[i], white );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, tabMat[i], red );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_PRESSED, tabMat[i], grey );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_DISABLED, tabMat[i], grey );

        //die borders! DIIIIEEEEE
        m_pTabs[i]->SetButtonBorderEnabled( false );
        m_pTabs[i]->SetPaintBorderEnabled( false );

        m_pTabs[i]->SetEnabled( tabEnabled[i] );

        //basic command stuff
        m_pTabs[i]->SetCommand( tabCmd[i] );
        //KeyValues *msg = new KeyValues("ButtonCommand");
        //msg->SetString("command", tabCmd[i]);
        //m_pTabs[i]->SetCommand( msg );
        //m_pTabs[i]->AddActionSignalTarget( pParent );
        //m_pTabs[i]->m_bHasTooltip = false;
        //m_pTabs[i]->SetTooltip( nullptr, "" );
    }

    m_pZombieGroups = new ComboBox( g_pZMView, "groupscombo", 5 , false ); 
    //m_pZombieGroups->SetOpenDirection( ComboBox::UP );
    m_pZombieGroups->SetText("None");
    m_pZombieGroups->GetMenu()->MakeReadyForUse();
    m_pZombieGroups->GetMenu()->SetBgColor( BLACK_BAR_COLOR );

    PositionButtons();
    PositionComboBox();
}

void CZMHudControlPanel::RemoveButtons()
{
    DevMsg("CZMHudControlPanel: removing buttons.\n");

    for (int i=0; i<NUM_BUTTONS; i++ )
    {
        if (m_pButtons[i] != NULL)
        {
            delete m_pButtons[i];
            m_pButtons[i] = NULL;
        }
    }

    m_ComboBoxItems.Purge();
    m_pZombieGroups->RemoveAll();
}

void CZMHudControlPanel::GroupsListUpdate()
{
    if ( m_iActiveTab != TAB_ZEDS ) return;


    //qck: Keep track of groups inside of our combo box. No duplicates.	
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    CUtlVector<int> vGroups;

    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        C_ZMBaseZombie* pZombie = g_pZombies->Element( i );
             
        if ( !pZombie ) continue;


        int group = pZombie->GetGroup();

        if ( group > INVALID_GROUP_INDEX && vGroups.Find( group ) == -1 )
        {
            vGroups.AddToTail( group );

            if ( m_ComboBoxItems.Find( group ) == -1 )
            {
                char groupName[16];
                Q_snprintf( groupName, sizeof( groupName ), "Group %i", group );

                KeyValues* kv = new KeyValues( "group" ); //qck: Associate entity serial number with its menu listing under key "serial"
                if ( !kv || !m_pZombieGroups ) return;
                
                kv->SetInt( "groupnum", group );
                m_pZombieGroups->AddItem( groupName, kv ); 
                kv->deleteThis();

                m_ComboBoxItems.AddToTail( group );
            }
        }
    }


    Menu* dropdown = m_pZombieGroups->GetMenu();
    if ( !dropdown ) return;


    for ( int i = 0; i < dropdown->GetItemCount(); i++ )
    {
        int index = dropdown->GetMenuID( i );

        KeyValues* kv = dropdown->GetItemUserData( index );

        if ( !kv ) continue;

        int group = kv->GetInt( "groupnum", INVALID_GROUP_INDEX );
        if ( group != INVALID_GROUP_INDEX && vGroups.Find( group ) == -1 )
        {
            dropdown->DeleteItem( index );
            m_ComboBoxItems.FindAndRemove( group );

            --i;

            //
            if ( !dropdown->GetItemCount() )
            {
                m_pZombieGroups->SetText( "None" );
                return;
            }
        }
    }
}

void CZMHudControlPanel::CreateGroup()
{
    // Find empty group.
    int newgroup = 1;

    for ( int i = 0; i < g_pZombies->Count(); )
    {
        C_ZMBaseZombie* pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetGroup() == newgroup )
        {
            i = 0;
            ++newgroup;

            continue;
        }

        ++i;
    }

    if ( newgroup <= MAX_GROUP_INDEX )
    {
        ZMClientUtil::SetSelectedGroup( newgroup );
    }
}

void CZMHudControlPanel::SelectGroup()
{
    if ( m_pZombieGroups )
    {
        KeyValues* kv = m_pZombieGroups->GetActiveItemUserData();

        if ( kv )
        {
            ZMClientUtil::SelectGroup( kv->GetInt( "groupnum", INVALID_GROUP_INDEX ) );
        }
    }
}

void CZMHudControlPanel::PositionButtons()
{
    //TGB: this is ugly and should be reworked to a keyvalues approach where the "slot" icon is in
    //	is specified, so we can have stuff like A _ C, where _ is an open space.

    //do the scaling
    //TGB: UNDONE: no more scaling
    //const float flVerScale = (float)ScreenHeight() / 480.0f;
    const float flVerScale = 1;

    //const int PANEL_TOPLEFT_X = HOR_ADJUST - PANEL_SPACING;
    const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
    //const int PANEL_TOPLEFT_Y = VER_ADJUST - PANEL_SPACING;
    const int PANEL_TOPLEFT_Y = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
    //-------
    //BUTTONS
    //-------

    const int scaledsize = (int)(BUTTON_SIZE * flVerScale);
    const int scaledspacing = (int)(BUTTON_SPACING * flVerScale);

    //working in unscaled values
    int start_x = PANEL_TOPLEFT_X + BUTTON_SPACING;
    int start_y = PANEL_TOPLEFT_Y + BUTTON_SPACING + 8; // Add a bit more spacing between tabs and buttons.
    //and now they're scaled
    start_x = (int)(start_x * flVerScale);
    start_y = (int)(start_y * flVerScale);

    //int x = start_x;
    //int y = start_y;

    //build array of per-tab start positions
    int tab_x[NUM_TABS];
    int tab_y[NUM_TABS];
    //keep track of how many buttons have been positioned per tab
    int tab_count[NUM_TABS];
    for (int i = 0; i < NUM_TABS; i++ )
    {
        tab_x[i] = start_x;
        tab_y[i] = start_y;
        tab_count[i] = 0;
    }

    int curTab = 0;
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if (!m_pButtons[i])
        {
            Warning("CZMHudControlPanel: Attempted to position nonexistant button.");
            return;
        }

        //look up tab for this button
        curTab = (int)buttonToTab[i];

        //apply scaled values
        m_pButtons[i]->SetPos( tab_x[curTab], tab_y[curTab] );
        m_pButtons[i]->SetSize( scaledsize, scaledsize );

        tab_count[curTab] += 1;

        //hardcoded line switch
        //0 1 2
        //3 4 5
        //6 7 8
        if ( tab_count[curTab] == 3 || tab_count[curTab] == 6)
        {
            tab_x[curTab] = start_x;
            tab_y[curTab] += scaledspacing + scaledsize;
        }
        else
        {
            tab_x[curTab] += scaledspacing + scaledsize;
        }
    }

    //-------
    //TABS
    //-------
    //unscaled values
    int tabpos_x = PANEL_TOPLEFT_X + BUTTON_SPACING;
    int tabpos_y = PANEL_TOPLEFT_Y - BUTTON_SIZE;
    //scale them...
    tabpos_x = (int)(tabpos_x * flVerScale);
    tabpos_y = (int)(tabpos_y * flVerScale);

    for (int i = 0; i < NUM_TABS; i++)
    {
        if (!m_pTabs[i])
        {
            Warning("CZMHudControlPanel: Attempted to position nonexistant tab.");
            return;
        }

        //apply scaled values
        m_pTabs[i]->SetPos( tabpos_x, tabpos_y );
        m_pTabs[i]->SetSize( scaledsize, scaledsize );

        tabpos_x += ( scaledsize + scaledspacing );

    }
}

void CZMHudControlPanel::PositionComboBox()
{
    //do the scaling
    //TGB: UNDONE: no more scaling
    //const float flVerScale = (float)ScreenHeight() / 480.0f;
    const float flVerScale = 1;

    //const int PANEL_TOPLEFT_X = HOR_ADJUST - PANEL_SPACING;
    const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
    //const int PANEL_TOPLEFT_Y = VER_ADJUST - PANEL_SPACING;
    const int PANEL_TOPLEFT_Y = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
    int combo_start_x = PANEL_TOPLEFT_X + COMBO_BOX_X_OFFSET;
    int combo_start_y = PANEL_TOPLEFT_Y + COMBO_BOX_Y_OFFSET;

    combo_start_x = (int)(combo_start_x * flVerScale);
    combo_start_y = (int)(combo_start_y * flVerScale);


    m_pZombieGroups->SetDrawWidth( COMBO_BOX_WIDTH );
    m_pZombieGroups->SetWide( COMBO_BOX_WIDTH );
    m_pZombieGroups->SetPos( combo_start_x, combo_start_y );
    m_pZombieGroups->SetVisible( false );
}

void CZMHudControlPanel::UpdateTabs( int activatedTab )
{
    //need to update our active tab?
    if ( activatedTab != -1 && activatedTab < NUM_TABS)
        m_iActiveTab = activatedTab;

    DevMsg("Tab set to %i\n", m_iActiveTab);

    //TGB: handle button visibility

    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if ( (int)buttonToTab[i] == m_iActiveTab )
            m_pButtons[i]->SetVisible( true  );
        else
            m_pButtons[i]->SetVisible( false );
    }

    //TGB: if our tab is buttonless, all buttons will be set to invisible now
    //this would be a good place to update visibility of other elements

    //qck: Take care of other element visibility

    if ( m_iActiveTab == TAB_ZEDS )
        m_pZombieGroups->SetVisible( true );
    else 
        m_pZombieGroups->SetVisible( false );

    //qck: Testing to see if KeyValues stay
    //if(m_pZombieGroups->GetActiveItemUserData() != NULL)
    //{
    //	KeyValues* kv = m_pZombieGroups->GetActiveItemUserData();
    //	int test = kv->GetInt("serial");
    //	DevMsg("Serial number of selected item: %i\n", test);
    //}
    
}

void CZMHudControlPanel::SetBgColor( const Color& clr )
{
    m_BgColor = clr;
}

void CZMHudControlPanel::SetFgColor( const Color& clr )
{
    m_FgColor = clr;

    for ( int i = 0; i < ARRAYSIZE( m_pButtons ); i++ )
    {
        if ( !m_pButtons[i] ) continue;

        m_pButtons[i]->GetBitmapImage( CBitmapButton::BUTTON_ENABLED )->SetColor( clr );
    }

    for ( int i = 0; i < ARRAYSIZE( m_pTabs ); i++ )
    {
        if ( !m_pTabs[i] ) continue;

        m_pTabs[i]->GetBitmapImage( CBitmapButton::BUTTON_ENABLED )->SetColor( clr );
    }
}

void CZMHudControlPanel::Paint()
{
    // Just paint background. Buttons are handled by viewport.
    int sizex = PANEL_SIZE_X + 30;
    int sizey = PANEL_SIZE_Y + 30;

    vgui::surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( ScreenWidth() - sizex, ScreenHeight() - sizey, ScreenWidth(), ScreenHeight() );
}
