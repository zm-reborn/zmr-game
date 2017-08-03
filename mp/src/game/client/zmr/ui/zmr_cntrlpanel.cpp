#include "cbase.h"

#include "vgui_controls/ComboBox.h"
#include "spectatorgui.h"



#include "zmr_cntrlpanel.h"


extern IGameUIFuncs *gameuifuncs; // for key binding details

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CZMControlPanel::CZMControlPanel( vgui::Panel *pParent )
{
    LoadButtons( pParent );

    UpdateTabs( 0 );
}

CZMControlPanel::~CZMControlPanel()
{
    RemoveButtons();
}

void CZMControlPanel::LoadButtons( vgui::Panel *pParent )
{
    const color32 white = { 255, 255, 255, 255 };
    const color32 grey = { 155, 155, 155, 255 };
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
        "MODE_GOTO_GROUP",
        "MODE_POWER_SPOTCREATE", //spot create
        "MODE_POWER_DELETEZOMBIES",
        "MODE_JUMP_CEILING",
    };

    //TGB: power costs are now printf'd into these, see toolTipCosts array below
    /*const char *toolTip[NUM_BUTTONS] =
    {
        "Explosion: Click in the world to blast objects away. \n[Cost: %i]",
        "Nightvision: Toggles your nightvision.",
        "Attack: Order selected units to attack any humans they see.",
        "Defend: Order selected units to defend their current location.",
        "Select all: Select all your zombies.",
        "Ambush: Set up an ambush using selected units. The units will stay put until a human comes near the ambush trigger.",
        "Create squad: Create a squad from selected units.",
        "Select squad: Select the chosen squad. The units in this squad will be selected.",
        "Hidden Summon: Click in the world to create a Shambler. Only works out of sight of the humans. \n[Cost: %i]",
        "Expire: Relinquish your control of the currently selected units.",
        "Banshee ceiling ambush: Order selected banshees to cling to the ceiling and hide until humans pass underneath."
    };*/

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

    DevMsg("CZMControlPanel: creating buttons.\n");

    //load buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        m_pButtons[i] = new CBitmapButton( pParent, buttonCmd[i], "" ); 
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED, buttonMat[i], white );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, buttonMat[i], red );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_PRESSED, buttonMat[i], grey );
        m_pButtons[i]->SetButtonBorderEnabled( false );
        m_pButtons[i]->SetPaintBorderEnabled( false );
        m_pButtons[i]->SetProportional( false );

        //basic command stuff
        m_pButtons[i]->SetCommand( buttonCmd[i] );
        
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

    //load tab buttons
    for (int i = 0; i < NUM_TABS; i++)
    {
        m_pTabs[i] = new CBitmapButton( pParent, tabCmd[i], "" ); 
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED, tabMat[i], white );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, tabMat[i], red );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_PRESSED, tabMat[i], grey );
        
        //die borders! DIIIIEEEEE
        m_pTabs[i]->SetButtonBorderEnabled( false );
        m_pTabs[i]->SetPaintBorderEnabled( false );
        
        //basic command stuff
        m_pTabs[i]->SetCommand( tabCmd[i] );
        //KeyValues *msg = new KeyValues("ButtonCommand");
        //msg->SetString("command", tabCmd[i]);
        //m_pTabs[i]->SetCommand( msg );
        //m_pTabs[i]->AddActionSignalTarget( pParent );
        //m_pTabs[i]->m_bHasTooltip = false;
        //m_pTabs[i]->SetTooltip( nullptr, "" );
    }

    m_pZombieGroups = new ComboBox(pParent, "groupscombo", 5 , false); 
    //m_pZombieGroups->SetOpenDirection( ComboBox::UP );
    m_pZombieGroups->SetText("None");
    m_pZombieGroups->GetMenu()->MakeReadyForUse();
    m_pZombieGroups->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
}

void CZMControlPanel::RemoveButtons()
{
    DevMsg("CZMControlPanel: removing buttons.\n");

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

void CZMControlPanel::GroupsListUpdate()
{
/*    //qck: Keep track of groups inside of our combo box. No duplicates.	
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if(pPlayer)
    {
        for(int i=0; i < pPlayer->m_ZombieGroupSerial.Count(); i++)
        {
            int serialNumber = pPlayer->m_ZombieGroupSerial[i];
             
            if( m_ComboBoxItems.Find( serialNumber ) == -1)
            {
                char groupName[ 16 ];
                Q_snprintf( groupName, sizeof( groupName ), "Group %i", i );

                KeyValues* kv = new KeyValues("group"); //qck: Associate entity serial number with its menu listing under key "serial"
                if (!kv || !m_pZombieGroups) return;
                
                kv->SetInt("serial", serialNumber);
                m_pZombieGroups->AddItem(groupName, kv); 
                kv->deleteThis();

                m_ComboBoxItems.AddToTail( serialNumber );

                DevMsg("Number of groups: %i\n", (i + 1));
            }
        }
    }
    */
}

//--------------------------------------------------------------
// TGB: remove a group from the dropdown list
//--------------------------------------------------------------
void CZMControlPanel::RemoveGroup(int serial)
{
    /*
    Menu *dropdown = m_pZombieGroups->GetMenu();
    if (!dropdown) return;

    // removing based purely on a bit of userdata turns out to be a hassle
    for (int i = 0; dropdown->GetItemCount(); i++)
    {
        int index = dropdown->GetMenuID(i);

        KeyValues *kv = dropdown->GetItemUserData(index);
        if (kv && kv->GetInt("serial") == serial)
        {
            dropdown->DeleteItem(index);
            DevMsg("Removed zombie group from menu\n");
            break;
        }
    }

    //more clearing up from the various tracking lists
    m_ComboBoxItems.FindAndRemove(serial);

    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if(pPlayer)
        pPlayer->m_ZombieGroupSerial.FindAndRemove(serial);


    //reset the text area of the combobox
    m_pZombieGroups->SetText("None");
    */
}

void CZMControlPanel::PositionButtons()
{
    //TGB: this is ugly and should be reworked to a keyvalues approach where the "slot" icon is in
    //	is specified, so we can have stuff like A _ C, where _ is an open space.

    //do the scaling
    //TGB: UNDONE: no more scaling
    //const float flVerScale = (float)ScreenHeight() / 480.0f;
    const float flVerScale = 1;

    const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
    const int PANEL_TOPLEFT_Y = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
    //-------
    //BUTTONS
    //-------

    const int scaledsize = (int)(BUTTON_SIZE * flVerScale);
    const int scaledspacing = (int)(BUTTON_SPACING * flVerScale);

    //working in unscaled values
    int start_x = PANEL_TOPLEFT_X + BUTTON_SPACING;
    int start_y = PANEL_TOPLEFT_Y + BUTTON_SPACING;
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
            Warning("CZMControlPanel: Attempted to position nonexistant button.");
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
            Warning("CZMControlPanel: Attempted to position nonexistant tab.");
            return;
        }

        //apply scaled values
        m_pTabs[i]->SetPos( tabpos_x, tabpos_y );
        m_pTabs[i]->SetSize( scaledsize, scaledsize );

        tabpos_x += ( scaledsize + scaledspacing );

    }
}

void CZMControlPanel::PositionComboBox()
{
    //do the scaling
    //TGB: UNDONE: no more scaling
    //const float flVerScale = (float)ScreenHeight() / 480.0f;
    const float flVerScale = 1;

    const int PANEL_TOPLEFT_X = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
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

void CZMControlPanel::OnCommand( const char* command )
{
    Msg( "OnCommand: %s\n", command );
}

void CZMControlPanel::UpdateTabs( int activatedTab )
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

//draw background onto given surface
void CZMControlPanel::PaintControls( vgui::ISurface *surface )
{
    if (!surface)
        return;

    //TGB: UNDONE: no more scaling
    //const float flVerScale = (float)ScreenHeight() / 480.0f;
//	const float flVerScale = 1;

    //TGB: replaced with unscaled method of positioning
    //const int x_tl = (int)(PANEL_TOPLEFT_X * flVerScale);
    //const int y_tl = (int)(PANEL_TOPLEFT_Y * flVerScale);
    //const int x_br = (int)(PANEL_BOTRIGHT_X * flVerScale);
    //const int y_br = (int)(PANEL_BOTRIGHT_Y * flVerScale);
    const int x_tl = ScreenWidth() - PANEL_SIZE_X + HOR_ADJUST - PANEL_SPACING;
    const int y_tl = ScreenHeight() - PANEL_SIZE_Y + VER_ADJUST - PANEL_SPACING;
    //the bottom is simpler
    const int x_br = ScreenWidth() + HOR_ADJUST - PANEL_SPACING;
    const int y_br = ScreenHeight() + VER_ADJUST - PANEL_SPACING;

    surface->DrawSetColor( Color( 70, 0, 0, 76 ) );
    surface->DrawFilledRect( x_tl, y_tl, x_br, y_br ); 

    //TGB: draw tab bgs
    int top_x = 0, top_y = 0;
    int bot_x = 0, bot_y = 0;
//	const int spacing = 0;
    for (int i = 0; i < NUM_TABS; i++)
    {
        if (!m_pTabs[i]) return;
        
        m_pTabs[i]->GetPos( top_x, top_y );
        int w, h;
        m_pTabs[i]->GetSize( w, h );
        bot_x = top_x + w;
        bot_y = top_y + h;
        
        if ( m_iActiveTab == i )
        {
            surface->DrawSetColor( Color( 70, 0, 0, 76 ) );
            m_pTabs[i]->SetAlpha( 255 );
        }
        else
        {
            m_pTabs[i]->SetAlpha( 100 );
            surface->DrawSetColor( Color( 70, 0, 0, 40 ) );
        }

        surface->DrawFilledRect( top_x, top_y, bot_x, bot_y ); 
    }
}
