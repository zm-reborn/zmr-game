#include "cbase.h"
#include "hud_macros.h"
#include "iclientmode.h"

#include <vgui_controls/Controls.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "zmr_hud_itemhistory.h"
#include "weapons/zmr_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_itemhistory_draw_time( "zm_cl_itemhistory_draw_time", "3", FCVAR_ARCHIVE, "How long an item is drawn in the history?" );
ConVar zm_cl_itemhistory_draw_denied_time( "zm_cl_itemhistory_draw_denied_time", "1.5", FCVAR_ARCHIVE, "How long an 'ammo denied' icon is drawn in the history?" );

DECLARE_HUDELEMENT( CZMHudItemHistory );
DECLARE_HUD_MESSAGE( CZMHudItemHistory, ItemPickup );
DECLARE_HUD_MESSAGE( CZMHudItemHistory, AmmoDenied );
DECLARE_HUD_MESSAGE( CZMHudItemHistory, ZMAmmoPickup );


CZMHudItemHistory::CZMHudItemHistory( const char* pszElementName ) : CHudElement( pszElementName ), BaseClass( g_pClientMode->GetViewport(), pszElementName )
{
    m_wszAmmoFullMsg[0] = NULL;
    m_bNeedsDraw = false;
    m_iCurrentHistorySlot = 0;

    SetHiddenBits( HIDEHUD_MISCSTATUS );
}

CZMHudItemHistory* CZMHudItemHistory::GetInstance()
{
    return GET_HUDELEMENT( CZMHudItemHistory );//static_cast<CZMHudItemHistory*>( gHUD.FindElement( "HudHistoryResource" ) );
}

void CZMHudItemHistory::ApplySchemeSettings( vgui::IScheme* pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );
    SetPaintBackgroundEnabled( false );

    // lookup text to display for ammo full message
    auto* pwszAmmoFullText = g_pVGuiLocalize->Find( "#hl2_AmmoFull" );
    if ( pwszAmmoFullText )
    {
        Q_wcsncpy( m_wszAmmoFullMsg, pwszAmmoFullText, sizeof( m_wszAmmoFullMsg ) );
    }
}

void CZMHudItemHistory::Init()
{
    HOOK_HUD_MESSAGE( CZMHudItemHistory, ItemPickup );
    HOOK_HUD_MESSAGE( CZMHudItemHistory, AmmoDenied );
    HOOK_HUD_MESSAGE( CZMHudItemHistory, ZMAmmoPickup );

    Reset();
}

void CZMHudItemHistory::Reset()
{
    m_PickupHistory.RemoveAll();
    m_iCurrentHistorySlot = 0;
}


float CZMHudItemHistory::GetItemSlotTypeDrawTime( ZMItemHistorySlotType_t type )
{
    if ( type == HISTSLOT_AMMODENIED )
    {
        return zm_cl_itemhistory_draw_denied_time.GetFloat();
    }

    return zm_cl_itemhistory_draw_time.GetFloat();
}

void CZMHudItemHistory::SetHistoryGap( int iNewHistoryGap )
{
}

void CZMHudItemHistory::AddWeaponToHistory( C_ZMBaseWeapon* pWeapon )
{
    int iId = pWeapon->entindex();

    // Don't show the same weapon twice.
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        auto& hist = m_PickupHistory[i];
        if ( hist.slotType == HISTSLOT_WEAP && hist.iId == iId )
        {
            // Update the display time, though.
            hist.flDisplayTime = gpGlobals->curtime + GetItemSlotTypeDrawTime( HISTSLOT_WEAP );
            m_bNeedsDraw = true;
            return;
        }
    }
    
    AddIconToHistory( HISTSLOT_WEAP, iId, pWeapon, 0, nullptr );
}

void CZMHudItemHistory::AddAmmoToHistory( int iAmmoIndex, int nCount )
{
    if ( nCount <= 0 )
        return;


    // Clear out any ammo pickup denied icons, since we can obviously pickup again
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        auto& hist = m_PickupHistory[i];
        if ( hist.slotType == HISTSLOT_AMMODENIED && hist.iId == iAmmoIndex )
        {
            hist.flDisplayTime = 0.0f;

            m_iCurrentHistorySlot = i;
            break;
        }
    }

    // If there already is history of this, update the existing one.
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        auto& hist = m_PickupHistory[i];
        if ( hist.slotType == HISTSLOT_AMMO && hist.iId == iAmmoIndex )
        {
            hist.nCount += nCount;
            return;
        }
    }

    AddIconToHistory( HISTSLOT_AMMO, iAmmoIndex, nullptr, nCount, nullptr );
}

void CZMHudItemHistory::AddIconToHistory( ZMItemHistorySlotType_t iType, int iId, C_ZMBaseWeapon* pWeapon, int nCount, CHudTexture* pIcon )
{
    // Check to see if the pic would have to be drawn too high. If so, start again from the bottom
    if ( (m_flHistoryGap * (m_iCurrentHistorySlot+1)) > GetTall() )
    {
        m_iCurrentHistorySlot = 0;
    }

    if ( m_PickupHistory.Count() < (m_iCurrentHistorySlot + 1) )
    {
        m_PickupHistory.EnsureCount( m_iCurrentHistorySlot + 1 );
        m_PickupHistory[m_iCurrentHistorySlot].slotType = HISTSLOT_EMPTY;
    }

    auto& freeSlot = m_PickupHistory[m_iCurrentHistorySlot];
    
    if ( freeSlot.slotType != HISTSLOT_EMPTY )
    {
        // Don't override existing pickup icons with denied icons
        if ( iType == HISTSLOT_AMMODENIED )
        {
            return;
        }
    }


    freeSlot.iId = iId;
    freeSlot.icon = pIcon;
    freeSlot.slotType = iType;
    freeSlot.m_hWeapon.Set( pWeapon );
    freeSlot.nCount = nCount;
    freeSlot.flDisplayTime = gpGlobals->curtime + GetItemSlotTypeDrawTime( iType );

    ++m_iCurrentHistorySlot;

    m_bNeedsDraw = true;
}

void CZMHudItemHistory::CheckClearHistory()
{
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        if ( m_PickupHistory[i].slotType != HISTSLOT_EMPTY )
            return;
    }

    m_iCurrentHistorySlot = 0;
}

bool CZMHudItemHistory::ShouldDraw()
{
    return m_bNeedsDraw && CHudElement::ShouldDraw();
}

void CZMHudItemHistory::Paint()
{
    // set when drawing should occur
    // will be set if valid drawing does occur
    m_bNeedsDraw = false;

    float flCurTime = gpGlobals->curtime;

    int wide, tall;
    GetSize( wide, tall );

    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        auto& hist = m_PickupHistory[i];

        if ( hist.slotType == HISTSLOT_EMPTY )
        {
            continue;
        }
            
        // Check expired
        if ( hist.flDisplayTime <= flCurTime )
        {  
            hist.slotType = HISTSLOT_EMPTY;
            CheckClearHistory();
            continue;
        }

        m_bNeedsDraw = true;

        float timeleft = hist.flDisplayTime - gpGlobals->curtime;
        float scale = timeleft * 255;
        int alpha = MIN( (int)scale, 255 );
        Color clr = gHUD.m_clrNormal;
        clr[3] = alpha;

        bool bUseAmmoFullMsg = false;

        const CHudTexture* itemIcon = nullptr;
        const CHudTexture* itemAmmoIcon = nullptr;
        int nAmount = 0;

        switch ( hist.slotType )
        {
        case HISTSLOT_AMMO:
        {
            // Get the weapon we belong to
            itemIcon = gWR.GetAmmoIconFromWeapon( hist.iId );
            itemAmmoIcon = nullptr;

            nAmount = hist.nCount;

            break;
        }
        case HISTSLOT_AMMODENIED:
        {
            itemIcon = gWR.GetAmmoIconFromWeapon( hist.iId );
            nAmount = 0;
            bUseAmmoFullMsg = true;
            // display as red
            clr = gHUD.m_clrCaution;	
            clr[3] = alpha;

            break;
        }
        case HISTSLOT_WEAP:
        {
            auto* pWeapon = hist.m_hWeapon.Get();
            if ( !pWeapon )
            {
                continue;
            }

            if ( !pWeapon->HasAmmo() )
            {
                // if the weapon doesn't have ammo, display it as red
                clr = gHUD.m_clrCaution;	
                clr[3] = alpha;
            }

            itemIcon = pWeapon->GetSpriteInactive();

            break;
        }
        case HISTSLOT_ITEM:
        {
            if ( !hist.iId )
                continue;

            itemIcon = hist.icon;

            break;
        }
        default:
            // Unknown history type
            Assert( 0 );
            break;
        }

        if ( !itemIcon )
            continue;

        int ypos = tall - (m_flHistoryGap * (i + 1));
        int xpos = wide - itemIcon->Width() - m_flIconInset;


        itemIcon->DrawSelf( xpos, ypos, clr );

        if ( itemAmmoIcon )
        {
            itemAmmoIcon->DrawSelf( xpos - ( itemAmmoIcon->Width() * 1.25f ), ypos, clr );
        }

        if ( nAmount )
        {
            wchar_t text[16];
            _snwprintf( text, sizeof( text ) / sizeof(wchar_t), L"%i", hist.nCount );

            // offset the number to sit properly next to the icon
            ypos -= ( vgui::surface()->GetFontTall( m_hNumberFont ) - itemIcon->Height() ) / 2;

            vgui::surface()->DrawSetTextFont( m_hNumberFont );
            vgui::surface()->DrawSetTextColor( clr );
            vgui::surface()->DrawSetTextPos( wide - m_flTextInset, ypos );
            vgui::surface()->DrawUnicodeString( text );
        }
        else if ( bUseAmmoFullMsg )
        {
            // offset the number to sit properly next to the icon
            ypos -= ( vgui::surface()->GetFontTall( m_hTextFont ) - itemIcon->Height() ) / 2;

            vgui::surface()->DrawSetTextFont( m_hTextFont );
            vgui::surface()->DrawSetTextColor( clr );
            vgui::surface()->DrawSetTextPos( wide - m_flTextInset, ypos );
            vgui::surface()->DrawUnicodeString( m_wszAmmoFullMsg );
        }
    }
}

void CZMHudItemHistory::MsgFunc_ItemPickup( bf_read& msg )
{
    Assert( 0 ); // TODO: Never used?

    char szName[64];
    msg.ReadString( szName, sizeof( szName ) );


    auto* pIcon = gHUD.GetIcon( szName );
    if ( !pIcon )
        return;

    // Add the item to the history
    AddIconToHistory( HISTSLOT_ITEM, 0, nullptr, 0, pIcon );
}

void CZMHudItemHistory::MsgFunc_AmmoDenied( bf_read& msg )
{
    int iAmmoIndex = msg.ReadShort();

    // see if there are any existing ammo items of that type
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        if ( m_PickupHistory[i].slotType == HISTSLOT_AMMO && m_PickupHistory[i].iId == iAmmoIndex )
        {
            // it's already in the list as a pickup, ignore
            return;
        }
    }

    // see if there are any denied ammo icons, if so refresh their timer
    for ( int i = 0; i < m_PickupHistory.Count(); i++ )
    {
        if ( m_PickupHistory[i].slotType == HISTSLOT_AMMODENIED && m_PickupHistory[i].iId == iAmmoIndex )
        {
            // it's already in the list, refresh
            m_PickupHistory[i].flDisplayTime = gpGlobals->curtime + GetItemSlotTypeDrawTime( HISTSLOT_AMMODENIED );
            m_bNeedsDraw = true;
            return;
        }
    }

    // add into the list
    AddIconToHistory( HISTSLOT_AMMODENIED, iAmmoIndex, nullptr, 0, nullptr );
}

void CZMHudItemHistory::MsgFunc_ZMAmmoPickup( bf_read& msg )
{
    int iAmmoIndex = msg.ReadShort();
    int nCount = msg.ReadShort();

    AddAmmoToHistory( iAmmoIndex, nCount );
}
