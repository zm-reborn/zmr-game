#pragma once


#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/TextImage.h>

#include "zmr/c_zmr_tips.h"


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


    void QueueTip( const char* name, float delay );
    void QueueTip( CZMTip* pTip, float delay );
    void QueueTip( int index, float delay );

    int SetMessageByName( const char* name, bool force = false );
    bool SetMessage( const char* msg, int index = 0, float displaytime = 5.0f, bool pulse = false, int priority = 0, const char* image = nullptr, bool bSound = false, int team = 0 );


    bool CanDisplay();
    void HideTooltip();
    
    int GetCurrentIndex() const { return m_iCurIndex; };

    bool IsDisplayingTip() const { return m_flAlphaMult > 0.0f; }
    bool CanBeOverriden() const { return m_iPriority != -1; }
    bool CanBeOverriden( int priority ) const { return (CanBeOverriden() && (m_iPriority < priority || m_iPriority == 0)); }


    



    void MsgFunc_ZMTooltip( bf_read& msg );

private:
    void SetText( const char* txt );


    int m_nTexId;
    vgui::TextImage* m_pTextImage;

    
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
