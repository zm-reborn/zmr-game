#pragma once


class CZMLineTool;

#include "zmr_boxselect.h"
#include "zmr_cntrlpanel.h"
#include "zmr_buildmenu.h"
#include "zmr_buildmenu_new.h"
#include "zmr_manimenu.h"
#include "zmr_manimenu_new.h"
#include "zmr_framepanel.h"


enum ZMClickMode_t
{
    ZMCLICKMODE_NORMAL = 0,
    ZMCLICKMODE_TRAP,
    ZMCLICKMODE_RALLYPOINT,
    ZMCLICKMODE_HIDDEN,
    ZMCLICKMODE_PHYSEXP,
    ZMCLICKMODE_AMBUSH,

    ZMCLICKMODE_MAX
};

#define DOUBLECLICK_DELTA           0.4f

class CZMFrame : public CHudElement, public CZMFramePanel

{
public:
    DECLARE_CLASS_SIMPLE( CZMFrame, CZMFramePanel );


    CZMFrame( const char* pElementName );
    ~CZMFrame();


    virtual void LevelInit() OVERRIDE;
    //virtual bool ShouldDraw() OVERRIDE;
    virtual void OnMouseReleased( MouseCode code ) OVERRIDE;
    //virtual void OnMouseMoved( MouseCode code ) OVERRIDE;
    virtual void OnCursorMoved( int, int ) OVERRIDE;
    virtual void OnMousePressed( MouseCode code ) OVERRIDE;
    //virtual void OnMouseDoublePressed( MouseCode code ) OVERRIDE;
    bool ShouldDraw() OVERRIDE { return IsVisible(); };
    virtual void OnThink() OVERRIDE;
    //virtual void PaintBackground() OVERRIDE;
    virtual void SetVisible( bool ) OVERRIDE;

    virtual void OnMouseWheeled( int ) OVERRIDE;
    //virtual void ShowPanel( const char* name ) OVERRIDE;
    //virtual void ShowPanel( bool state ) OVERRIDE;

    void ShowPanel( bool state ) { if ( IsVisible() == state ) return; SetVisible( state ); };
    
    inline ZMClickMode_t GetClickMode() { return m_iClickMode; };
    void SetClickMode( ZMClickMode_t, bool print = true );


    CZMBuildMenuBase* GetBuildMenu();
    CZMManiMenuBase* GetManiMenu();

    inline bool IsDoubleClickLeft() { return ( gpGlobals->curtime - m_flLastLeftClick ) < DOUBLECLICK_DELTA; };
    inline bool IsDoubleClickRight() { return ( gpGlobals->curtime - m_flLastRightClick ) < DOUBLECLICK_DELTA; };

private:
    void TraceScreenToWorld( int, int, trace_t*, CTraceFilterSimple*, int );
    
    void OnLeftClick();
    void OnRightClick();

    void OnLeftRelease();
    void OnRightRelease();

    void FindZombiesInBox( int, int, int, int, bool );
    void FindZMObject( int, int, bool );
    void DoMoveLine();

    void CloseChildMenus();

    CZMBoxSelect* m_BoxSelect;
    CZMLineTool* m_LineTool;
    CZMHudControlPanel* m_pZMControl;
    CZMManiMenu* m_pManiMenu;
    CZMManiMenuNew* m_pManiMenuNew;
    CZMBuildMenu* m_pBuildMenu;
    CZMBuildMenuNew* m_pBuildMenuNew;

    MouseCode m_MouseDragStatus;

    ZMClickMode_t m_iClickMode;

    float m_flLastLeftClick;
    float m_flLastRightClick;
};

extern CZMFrame* g_pZMView;
