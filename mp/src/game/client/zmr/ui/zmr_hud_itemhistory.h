#pragma once

#include "hudelement.h"
#include "ehandle.h"

#include <vgui_controls/Panel.h>


namespace vgui
{
    class IScheme;
}

class C_ZMBaseWeapon;
class CHudTexture;


enum ZMItemHistorySlotType_t
{
    HISTSLOT_EMPTY = 0,

    HISTSLOT_AMMO,
    HISTSLOT_WEAP,
    HISTSLOT_ITEM,

    HISTSLOT_AMMODENIED,
};

//
// Item history, ie. weapon & ammo pickup notifications.
//
class CZMHudItemHistory : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudItemHistory, vgui::Panel );

    CZMHudItemHistory( const char* pszElementName );

    virtual void Init() OVERRIDE;
    virtual void Reset() OVERRIDE;

    virtual bool ShouldDraw() OVERRIDE;

    virtual void Paint() OVERRIDE;

    virtual void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;


    static float GetItemSlotTypeDrawTime( ZMItemHistorySlotType_t type );

    static CZMHudItemHistory* GetInstance();


    void    AddWeaponToHistory( C_ZMBaseWeapon* pWeapon );
    void    AddAmmoToHistory( int iAmmoIndex, int nCount );
    
    void    CheckClearHistory();
    void    SetHistoryGap( int iNewHistoryGap );
    void    AddIconToHistory( ZMItemHistorySlotType_t iType, int iId, C_ZMBaseWeapon* pWeapon, int nCount, CHudTexture* pIcon );


    void    MsgFunc_ItemPickup( bf_read& msg );
    void    MsgFunc_ZMAmmoPickup( bf_read& msg );
    void    MsgFunc_AmmoDenied( bf_read& msg );

private:
    // these vars are for hl1-port compatibility
    int     m_iHistoryGap;
    int     m_iCurrentHistorySlot;
    wchar_t m_wszAmmoFullMsg[32];
    bool    m_bNeedsDraw;


    struct HistoryData_t
    {
        HistoryData_t() 
        { 
            // init this here, because the code that overwrites previous history items will use this
            // to check to see if the item is empty
            flDisplayTime = 0.0f; 
        }

        ZMItemHistorySlotType_t slotType;

        float                   flDisplayTime; // the time at which this item should be removed from the history
        
        int                     nCount; // Ammo count
        int                     iId; // Ent index or ammo index.

        CHandle<C_ZMBaseWeapon> m_hWeapon;

        CHudTexture *icon;
    };

    CUtlVector<HistoryData_t>   m_PickupHistory;


    CPanelAnimationVarAliasType( float, m_flHistoryGap, "history_gap", "42", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flIconInset, "icon_inset", "28", "proportional_float" );
    CPanelAnimationVarAliasType( float, m_flTextInset, "text_inset", "26", "proportional_float" );
    CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudNumbersSmall" );
    CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};
