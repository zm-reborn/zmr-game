#include "cbase.h"

#include <vgui_controls/ComboBox.h>
#include "spectatorgui.h"
#include "iclientmode.h"
#include "vgui_bitmapbutton.h"


#include "npcs/zmr_zombiebase_shared.h"
#include "zmr_zmview_base.h"
#include "zmr_cntrlpanel.h"
#include "c_zmr_zmvision.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

const int buttonToTab[CZMHudControlPanel::NUM_BUTTONS] = {
    CZMHudControlPanel::TAB_POWERS, //spotcreate
    CZMHudControlPanel::TAB_POWERS, //night vision
    CZMHudControlPanel::TAB_MODES, //select all
    CZMHudControlPanel::TAB_MODES, //defense
    CZMHudControlPanel::TAB_MODES, //offense 
    CZMHudControlPanel::TAB_MODES, //ambush
    CZMHudControlPanel::TAB_ZEDS, //create group
    CZMHudControlPanel::TAB_ZEDS, //goto group
    CZMHudControlPanel::TAB_POWERS, //physexp
    CZMHudControlPanel::TAB_POWERS, //delete
    CZMHudControlPanel::TAB_MODES //ceiling ambush
};

CZMHudControlPanel::CZMHudControlPanel( Panel* pParent ) : Panel( pParent, "CZMHudControlPanel" )
{
    SetProportional( true );
    SetMouseInputEnabled( true );
    DisableMouseInputForThisPanel( true );
    SetKeyBoardInputEnabled( false );


    CreateButtons();


    UpdateTabs( CZMHudControlPanel::TAB_MODES );


    m_nTexBgId = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_nTexBgId, "zmr_effects/hud_bg_zmcntrl", true, false );
}

void CZMHudControlPanel::ApplySchemeSettings( IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );


    SetBgColor( GetSchemeColor( "ZMHudBgColor", pScheme ) );
    SetFgColor( GetSchemeColor( "ZMFgColor", pScheme ) );
}

void CZMHudControlPanel::PerformLayout()
{
    PositionButtons();
    PositionComboBox();
}

void CZMHudControlPanel::OnThink()
{
    if ( !IsVisible() ) return;


    GroupsListUpdate();
}

void CZMHudControlPanel::OnCommand( const char* command )
{
    if ( Q_stricmp( command, "TAB_POWERS" ) == 0 )
    {
        UpdateTabs( CZMHudControlPanel::TAB_POWERS );
    }
    else if ( Q_stricmp( command, "TAB_MODES" ) == 0 )
    {
        UpdateTabs( CZMHudControlPanel::TAB_MODES );
    }
    else if ( Q_stricmp( command, "TAB_ZEDS" ) == 0 )
    {
        UpdateTabs( CZMHudControlPanel::TAB_ZEDS );
    }
    else if ( Q_stricmp( command, "MODE_SELECT_ALL" ) == 0 )
    {
        ZMClientUtil::SelectAllZombies();
    }
    else if ( Q_stricmp( command, "MODE_DEFENSIVE" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_zombiemode %i", ZOMBIEMODE_DEFEND ) );
    }
    else if ( Q_stricmp( command, "MODE_OFFENSIVE" ) == 0 )
    {
        engine->ClientCmd( VarArgs( "zm_cmd_zombiemode %i", ZOMBIEMODE_OFFENSIVE ) );
    }
    else if ( Q_stricmp( command, "MODE_POWER_DELETEZOMBIES" ) == 0 )
    {
        engine->ClientCmd( "zm_cmd_delete" );
    }
    else if ( Q_stricmp( command, "MODE_POWER_SPOTCREATE" ) == 0 )
    {
        if ( g_pZMView ) g_pZMView->SetClickMode( ZMCLICKMODE_HIDDEN );
    }
    else if ( Q_stricmp( command, "MODE_POWER_PHYSEXP" ) == 0 )
    {
        if ( g_pZMView ) g_pZMView->SetClickMode( ZMCLICKMODE_PHYSEXP );
    }
    else if ( Q_stricmp( command, "MODE_AMBUSH_CREATE" ) == 0 )
    {
        if ( g_pZMView ) g_pZMView->SetClickMode( ZMCLICKMODE_AMBUSH );
    }
    else if ( Q_stricmp( command, "MODE_JUMP_CEILING" ) == 0 )
    {
        engine->ClientCmd( "zm_cmd_bansheeceiling" );
    }
    else if ( Q_stricmp( command, "MODE_POWER_NIGHTVISION" ) == 0 )
    {
        g_ZMVision.Toggle();
    }
    else if ( Q_stricmp( command, "MODE_SELECT_GROUP" ) == 0 )
    {
        SelectGroup();
    }
    else if ( Q_stricmp( command, "MODE_CREATE_GROUP" ) == 0 )
    {
        CreateGroup();
    }

    BaseClass::OnCommand( command );
}

void CZMHudControlPanel::CreateButtons()
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
        "VGUI/minispotcreate",					//spot create
        "VGUI/minieye",							//night vision
        "VGUI/miniselectall",					//select all
        "VGUI/minishield",						//defensive mode
        "VGUI/minicrosshair",					//offensive mode
        "VGUI/miniceiling",						//banshee ceiling jump/ambush
        "VGUI/minigroupadd",					//create group
        "VGUI/minigroupselect",					//select group
        "VGUI/minishockwave",					//physexp
        "VGUI/minideletezombies",				//Delete Zombies
        "VGUI/miniarrows",						//ambush mode
    };

    const char *buttonCmd[NUM_BUTTONS] =
    {
        "MODE_POWER_SPOTCREATE",
        "MODE_POWER_NIGHTVISION",
        "MODE_SELECT_ALL",
        "MODE_DEFENSIVE",
        "MODE_OFFENSIVE",
        "MODE_JUMP_CEILING",
        "MODE_CREATE_GROUP",
        "MODE_SELECT_GROUP",
        "MODE_POWER_PHYSEXP",
        "MODE_POWER_DELETEZOMBIES",
        "MODE_AMBUSH_CREATE",
    };

    const char *toolTip[NUM_BUTTONS] =
    {
        "zmmenu_createhidden",
        "zmmenu_nv",
        "zmmenu_selectall",
        "zmmenu_defend",
        "zmmenu_attack",
        "zmmenu_bansheeceil",
        "zmmenu_creategroup",
        "zmmenu_selectgroup",
        "zmmenu_exp",
        "zmmenu_delete",
        "zmmenu_ambush",
    };

    for (int i = 0; i < ARRAYSIZE( m_pButtons ); i++)
    {
        m_pButtons[i] = new CZMBitMapButton( this, buttonCmd[i], "" );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED, buttonMat[i], white );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, buttonMat[i], red );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_PRESSED, buttonMat[i], grey );
        m_pButtons[i]->SetImage( CBitmapButton::BUTTON_DISABLED, buttonMat[i], grey );
        m_pButtons[i]->SetButtonBorderEnabled( false );
        m_pButtons[i]->SetPaintBorderEnabled( false );
        m_pButtons[i]->SetCommand( buttonCmd[i] );
        m_pButtons[i]->SetTooltipName( toolTip[i] );
        m_pButtons[i]->AddActionSignalTarget( this );
    }

    //-------
    // TABS
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

    for (int i = 0; i < ARRAYSIZE( m_pTabs ); i++)
    {
        m_pTabs[i] = new CZMBitMapButton( this, tabCmd[i], "" );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED, tabMat[i], white );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_ENABLED_MOUSE_OVER, tabMat[i], red );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_PRESSED, tabMat[i], grey );
        m_pTabs[i]->SetImage( CBitmapButton::BUTTON_DISABLED, tabMat[i], grey );
        m_pTabs[i]->SetButtonBorderEnabled( false );
        m_pTabs[i]->SetPaintBorderEnabled( false );
        m_pTabs[i]->SetCommand( tabCmd[i] );
        m_pTabs[i]->AddActionSignalTarget( this );
    }

    m_pZombieGroups = new ComboBox( this, "groupscombo", 5 , false ); 
    //m_pZombieGroups->SetOpenDirection( ComboBox::UP );
    m_pZombieGroups->SetText("None");
    m_pZombieGroups->GetMenu()->MakeReadyForUse();
    m_pZombieGroups->GetMenu()->SetBgColor( BLACK_BAR_COLOR );


    InvalidateLayout();
}

void CZMHudControlPanel::GroupsListUpdate()
{
    if ( m_iActiveTab != TAB_ZEDS ) return;


    //qck: Keep track of groups inside of our combo box. No duplicates.	
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if ( !pPlayer ) return;


    CUtlVector<int> vGroups;

    g_ZombieManager.ForEachZombie( [ &vGroups, this ]( C_ZMBaseZombie* pZombie )
    {
        int group = pZombie->GetGroup();

        if ( group > INVALID_GROUP_INDEX && vGroups.Find( group ) == -1 )
        {
            vGroups.AddToTail( group );

            if ( m_ComboBoxItems.Find( group ) == -1 )
            {
                char groupName[16];
                Q_snprintf( groupName, sizeof( groupName ), "Group %i", group );

                KeyValuesAD kv( "group" ); //qck: Associate entity serial number with its menu listing under key "serial"
                if ( !kv || !m_pZombieGroups ) return;
                
                kv->SetInt( "groupnum", group );
                m_pZombieGroups->AddItem( groupName, kv ); 

                m_ComboBoxItems.AddToTail( group );
            }
        }
    } );


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
    
    for ( int i = 0; i < g_ZombieManager.GetNumZombies(); )
    {
        C_ZMBaseZombie* pZombie = g_ZombieManager.GetZombieByIndex( i );

        if ( pZombie->GetGroup() == newgroup )
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
    const int button_size = QuickPropScale( BUTTON_SIZE );
    const int button_spacing = QuickPropScale( BUTTON_SPACING );
    const int margin_top = QuickPropScale( MARGIN_TOP );
    const int margin_left = QuickPropScale( MARGIN_LEFT );
    const int margin_bottomright = QuickPropScale( MARGIN_BOTTOM_RIGHT );
    const int tab_spacing_y = QuickPropScale( TAB_SPACING );

    // 3x3 + spacing between them.
    const int panel_size_x = button_size * 3 + button_spacing * 2;
    const int panel_size_y = button_size * 3 + button_spacing + tab_spacing_y;

    SetSize( panel_size_x + margin_left + margin_bottomright, panel_size_y + margin_top + margin_bottomright );

    int x, y, w, h;
    GetParent()->GetBounds( x, y, w, h );

    SetPos( w - GetWide(), h - GetTall() );
    
    int topleft_x = margin_left;
    int topleft_y = margin_top;

    int start_x = topleft_x + 2 * button_size + button_spacing * 2;
    int start_y = topleft_y + button_size + tab_spacing_y; // Add a bit more spacing between tabs and buttons.

    int tab_x[NUM_TABS];
    int tab_y[NUM_TABS];
    int tab_count[NUM_TABS];
    for (int i = 0; i < NUM_TABS; i++ )
    {
        tab_x[i] = start_x;
        tab_y[i] = start_y;
        tab_count[i] = 0;
    }

    int curTab = 0;
    for (int i = 0; i < ARRAYSIZE( m_pButtons ); i++)
    {
        // Look up tab for this button
        curTab = buttonToTab[i];

        m_pButtons[i]->SetPos( tab_x[curTab], tab_y[curTab] );
        m_pButtons[i]->SetSize( button_size, button_size );

        tab_count[curTab] += 1;

        // Hardcoded line switch
        if ( tab_count[curTab] == 3 || tab_count[curTab] == 6)
        {
            tab_x[curTab] = start_x;
            tab_y[curTab] += button_spacing + button_size;
        }
        else
        {
            tab_x[curTab] -= button_spacing + button_size;
        }
    }

    //-------
    // TABS
    //-------
    int tabpos_x = topleft_x;
    int tabpos_y = topleft_y;

    for (int i = 0; i < ARRAYSIZE( m_pTabs ); i++)
    {
        m_pTabs[i]->SetPos( tabpos_x, tabpos_y );
        m_pTabs[i]->SetSize( button_size, button_size );

        tabpos_x += button_size + button_spacing;
    }
}

void CZMHudControlPanel::PositionComboBox()
{
    const int button_size = QuickPropScale( BUTTON_SIZE );
    const int button_spacing = QuickPropScale( BUTTON_SPACING );
    const int margin_top = QuickPropScale( MARGIN_TOP );
    const int margin_left = QuickPropScale( MARGIN_LEFT );
    const int margin_bottomright = QuickPropScale( MARGIN_BOTTOM_RIGHT );
    const int tab_spacing_y = QuickPropScale( TAB_SPACING );

    // 3x3 + spacing between them.
    const int panel_size_x = button_size * 3 + button_spacing * 2;
    const int panel_size_y = button_size * 3 + button_spacing + tab_spacing_y;

    const int topleft_x = margin_left;
    const int topleft_y = margin_top;

    int combo_start_x = topleft_x;
    int combo_start_y = topleft_y + button_size * 2 + button_spacing + tab_spacing_y + button_size / 4;

    int wide = panel_size_x;
    m_pZombieGroups->SetDrawWidth( wide );
    m_pZombieGroups->SetWide( wide );
    m_pZombieGroups->SetPos( combo_start_x, combo_start_y );
    m_pZombieGroups->SetVisible( false );
}

void CZMHudControlPanel::UpdateTabs( int activatedTab )
{
    if ( activatedTab != -1 && activatedTab < NUM_TABS )
    {
        m_iActiveTab = activatedTab;
    }

    for (int i = 0; i < ARRAYSIZE( m_pButtons ); i++)
    {
        m_pButtons[i]->SetVisible( buttonToTab[i] == m_iActiveTab );
    }

    m_pZombieGroups->SetVisible( m_iActiveTab == TAB_ZEDS );
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

void CZMHudControlPanel::PaintBackground()
{
    vgui::surface()->DrawSetColor( m_BgColor );
    surface()->DrawSetTexture( m_nTexBgId );
    surface()->DrawTexturedRect( 0, 0, GetWide(), GetTall() );
}
