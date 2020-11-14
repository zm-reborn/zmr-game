#pragma once

#include <clientscoreboarddialog.h>
#include <vgui/IImage.h>


#include "zmr_framepanel.h"
#include "c_zmr_importancesystem.h"

class CZMListRow;
class CZMListSection;
class CZMListPanel;
class CAvatarImage;

struct zm_avatarimg_t
{
    zm_avatarimg_t( CSteamID id, CAvatarImage* img )
    {
        SteamId = id;
        pImage = img;
    }

    CSteamID        SteamId;
    CAvatarImage*   pImage;
};

class CZMAvatarList
{
public:
    CZMAvatarList() { m_ImageList.Purge(); };


    int CreateAvatarBySteamId( CSteamID id );
    int FindAvatarBySteamId( CSteamID id );

    CAvatarImage*   GetImage( int i ) { return m_ImageList[i].pImage; };

private:
    CUtlVector<zm_avatarimg_t>   m_ImageList;
};



class CZMClientScoreBoardDialog : public CZMFramePanel, public IViewPortPanel, public CGameEventListener
{
private:
    DECLARE_CLASS_SIMPLE( CZMClientScoreBoardDialog, CZMFramePanel );
    
public:
    CZMClientScoreBoardDialog( IViewPort* pViewPort );
    ~CZMClientScoreBoardDialog();


    virtual void PerformLayout() OVERRIDE;
    virtual void PaintBackground() OVERRIDE;

    virtual const char* GetName() OVERRIDE { return PANEL_SCOREBOARD; };
    virtual void        SetData( KeyValues *data) OVERRIDE {};
    virtual bool        HasInputElements() OVERRIDE { return true; };
    vgui::VPANEL        GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); };
    virtual bool        IsVisible() OVERRIDE { return BaseClass::IsVisible(); };
    virtual void        SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); };


    virtual void        ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    virtual void        FireGameEvent( IGameEvent* event ) OVERRIDE;
    
    virtual void        OnThink() OVERRIDE;
    virtual void        Reset() OVERRIDE;
    virtual void        Update() OVERRIDE;
    virtual bool        NeedsUpdate() OVERRIDE;
    virtual void        ShowPanel( bool bShow ) OVERRIDE;


    MESSAGE_FUNC_PARAMS( OnListLayout, "OnListLayout", kv );
    MESSAGE_FUNC_PARAMS( OnRowItemPressed, "OnRowItemPressed", kv );

private:
    void UpdateStats(); // Stuff that is static and doesn't change during the map.
    void UpdateMapStats(); // Stuff that may change while we're playing.

    void    ToggleVoiceMute( int playerIndex );
    int     FindPlayerItem( int playerIndex );
    int     TeamToSection( int iTeam );

    void UpdateScoreboard();

    void UpdatePlayerInfo();
    void GetPlayerScoreInfo( int playerIndex, KeyValues* kv );
    void UpdatePlayerAvatar( int playerIndex, KeyValues* kv );


    CZMListPanel* m_pList;

    vgui::IImage* m_pImportanceImages[ZMIMPORTANCE_MAX];

    int     m_iPlayerIndexSymbol;
    float   m_flNextUpdateTime;
    float   m_flLastMouseToggle;

    int     m_iSectionZM;
    int     m_iSectionHuman;

    int     m_nTexBgSideId;
    int     m_nTexBgTopId;
    int     m_nTexBgCornerId;

    
    CZMAvatarList  m_Avatars;
    int m_iVoiceOff;
    int m_iVoiceOn;
};
