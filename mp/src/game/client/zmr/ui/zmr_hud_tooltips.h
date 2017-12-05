#pragma once


#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/TextImage.h>


enum TipParamType_t
{
    TIPPARAMTYPE_NONE = 0,

    TIPPARAMTYPE_KEY,
    TIPPARAMTYPE_CVAR,
};

struct ZMTipQueue_T
{
    int m_iIndex;
    float m_flDisplay;
};

class CZMTip
{
public:
    CZMTip( KeyValues* kv, int index );
    ~CZMTip();
    

    void LoadUsed( KeyValues* kv );
    void WriteUsed( KeyValues* kv );


    bool ShouldShow();

    void Reset();

    const char* GetName();
    void SetName( const char* name );

    void FormatMessage( char* buffer, int len );
    const char* GetMessage();
    void SetMessage( const char* msg );
    const char* GetIcon();
    void SetIcon( const char* icon );
    
    float GetTime() { return m_flTime; };
    bool GetPulse() { return m_bPulse; };
    int GetPriority() { return m_iPriority; };
    int GetTeam() { return m_iTeam; };
    bool DoSound() { return m_bSound; };
    bool CanBeQueued() { return m_bQueue; };
    bool ShowRandom() { return m_bRandom; };
    int GetLimit() { return m_nLimit; };

    int GetIndex() { return m_iIndex; };


    void IncShown() { m_nShown++; m_nShownPerGame++; };


    static TipParamType_t TipNameToType( const char* );
    
private:
    void SetParam( const char* );


    char* m_pszName;
    char* m_pszMessage;


    int m_iIndex;
    char* m_pszIcon;

    int m_iTeam;
    float m_flTime;
    int m_iPriority;
    bool m_bQueue;
    bool m_bRandom;
    bool m_bPulse;
    bool m_bSound;
    int m_nLimit;
    int m_nLimitPerGame;

    int m_nShown;
    int m_nShownPerGame;


    char* m_pszParam;
    TipParamType_t m_iParamType;
};

class CZMHudTooltip : public CHudElement, public vgui::Panel
{
public:
    DECLARE_CLASS_SIMPLE( CZMHudTooltip, vgui::Panel );

    CZMHudTooltip( const char* pElementName );
    ~CZMHudTooltip();


    virtual void Init() OVERRIDE;
    virtual void LevelInit() OVERRIDE;
    virtual void LevelShutdown() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE;


    void FindNextQueueTip();
    void FindNextRandomTipToQueue();


    void QueueTip( CZMTip*, float delay );
    void QueueTip( int index, float delay );
    CZMTip* FindMessageByName( const char* name );
    CZMTip* FindMessageByIndex( int index );
    int SetMessageByName( const char* name, bool force = false );
    bool SetMessage( const char* msg, int index = 0, float displaytime = 5.0f, bool pulse = false, int priority = 0, const char* image = nullptr, bool bSound = false, int team = 0 );


    bool CanDisplay();
    void HideTooltip();
    
    int GetCurrentIndex() { return m_iCurIndex; };

    bool IsDisplayingTip() { return m_flAlphaMult > 0.0f; };
    bool CanBeOverriden() { return m_iPriority != -1; };
    bool CanBeOverriden( int priority ) { return (CanBeOverriden() && (m_iPriority < priority || m_iPriority == 0)); };


    void SaveUsed();



    void MsgFunc_ZMTooltip( bf_read& msg );

private:
    void AddTip( KeyValues* kv );
    void SetText( const char* txt );


    void LoadTips();
    void LoadUsed();


    int m_nTexId;
    vgui::TextImage* m_pTextImage;

    CUtlVector<CZMTip*> m_vTips;
    CUtlVector<ZMTipQueue_T> m_vQueue;


    float m_flNextRandomTip;
    float m_flNextHide;
    float m_flNextSound;
    int m_nTexSize;

    int m_iCurIndex;

    bool m_bPulse;
    int m_iPriority;
    int m_iTeam;


    CPanelAnimationVar( vgui::HFont, m_hFont, "TooltipFont", "ZMHudTooltip" );
    CPanelAnimationVar( Color, m_BgColor, "TooltipBgColor", "70 0 0 150" );

    CPanelAnimationVar( float, m_flAlphaMult, "TooltipAlphaMult", "0" );
};
