#pragma once


#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/TextImage.h>


class CZMTip
{
public:
    CZMTip( KeyValues* kv, int index );
    ~CZMTip();
  

    void Reset();

    const char* GetName();
    void SetName( const char* name );
    const char* GetMessage();
    void SetMessage( const char* msg );
    const char* GetIcon();
    void SetIcon( const char* icon );
    
    float GetTime() { return m_flTime; };
    bool GetPulse() { return m_bPulse; };
    int GetPriority() { return m_iPriority; };
    int GetTeam() { return m_iTeam; };
    bool DoSound() { return m_bSound; };

    int GetIndex() { return m_iIndex; };
    
private:
    char* m_pszName;
    char* m_pszMessage;


    int m_iIndex;
    char* m_pszIcon;

    int m_iTeam;
    float m_flTime;
    int m_iPriority;
    bool m_bQueue;
    bool m_bPulse;
    bool m_bSound;
};

class CZMHudTooltip : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudTooltip, vgui::Panel );

    CZMHudTooltip( const char* pElementName );
    ~CZMHudTooltip();


    virtual void Init() OVERRIDE;
    virtual void LevelInit() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;


    CZMTip* FindMessageByName( const char* name );
    int SetMessageByName( const char* name );
    void SetMessage( const char* msg, int index = 0, float displaytime = 5.0f, bool pulse = false, int priority = 0, const char* image = nullptr, bool bSound = false, int team = 0 );


    bool CanDisplay();
    void HideTooltip();
    
    int GetCurrentIndex() { return m_iCurIndex; };

    bool IsDisplayingTip() { return m_flNextHide != 0.0f; };
    bool CanBeOverriden() { return m_iPriority != -1; };
    bool CanBeOverriden( int priority ) { return (CanBeOverriden() && (m_iPriority < priority || m_iPriority == 0)); };


    void MsgFunc_ZMTooltip( bf_read& msg );

private:
    void AddTip( KeyValues* kv );
    void SetText( const char* txt );


    int m_nTexId;
    vgui::TextImage* m_pTextImage;

    CUtlVector<CZMTip*> m_vTips;
    CUtlVector<int> m_vQueue;


    int m_iCurIndex;

    bool m_bPulse;
    float m_flNextHide;
    int m_iPriority;
    int m_iTeam;


    CPanelAnimationVar( vgui::HFont, m_hFont, "TooltipFont", "ZMHudTooltip" );
    CPanelAnimationVar( Color, m_BgColor, "TooltipBgColor", "70 0 0 150" );

    CPanelAnimationVar( float, m_flAlphaMult, "TooltipAlphaMult", "0" );
};
