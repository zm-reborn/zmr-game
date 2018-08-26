#pragma once


#include "cbase.h"
#include "hudelement.h"

#include "zmr_boxselect.h"
#include "zmr_linetool.h"
#include "zmr_buildmenu_base.h"
#include "zmr_manimenu_base.h"
#include "zmr_framepanel.h"




#define MASK_ZMVIEW             ( CONTENTS_SOLID | CONTENTS_MOVEABLE ) // When testing box select.
#define MASK_ZMSELECTUSABLE     ( MASK_SOLID & ~(CONTENTS_WINDOW|CONTENTS_GRATE) ) // When left clicking (not setting rallypoint, etc.)
#define MASK_ZMTARGET           MASK_SOLID // When right clicking.
#define MASK_ZMCREATE           MASK_SOLID // When left clicking.




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


class CZMViewBase : public CHudElement, public CZMFramePanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMViewBase, CZMFramePanel );


    CZMViewBase( const char* pElementName );
    ~CZMViewBase();


    // CHudElement
    virtual void LevelInit() OVERRIDE;
    virtual bool ShouldDraw() OVERRIDE;

    // Panel
    virtual void OnThink() OVERRIDE;
    virtual void SetVisible( bool visible ) OVERRIDE;
    virtual bool IsVisible() OVERRIDE; // Need to override some functions to make them public
private:
    virtual void OnCursorMoved( int x, int y ) OVERRIDE;
    virtual void OnMouseReleased( MouseCode code ) OVERRIDE;
    virtual void OnMousePressed( MouseCode code ) OVERRIDE;
    virtual void OnMouseWheeled( int delta ) OVERRIDE;
public:



    virtual void CloseChildMenus();
    virtual void HideMouseTools();


    virtual CZMBuildMenuBase* GetBuildMenu() { return nullptr; }
    virtual CZMManiMenuBase* GetManiMenu() { return nullptr; }


    
    ZMClickMode_t GetClickMode() const { return m_iClickMode; }
    virtual void SetClickMode( ZMClickMode_t mode, bool print = true );


    virtual float GetDoubleClickDelta() const;
    bool IsDoubleClickLeft() const;
    bool IsDoubleClickRight() const;

    bool IsDraggingLeft() const;
    bool IsDraggingRight() const;



    static void TraceScreenToWorld( int mx, int my, trace_t* res, CTraceFilterSimple* filter, int mask );


protected:

    virtual void OnLeftClick();
    virtual void OnRightClick();

    virtual void OnLeftRelease();
    virtual void OnRightRelease();




    void FindZombiesInBox( int start_x, int start_y, int end_x, int end_y, bool bSticky );
    void FindZMObject( int x, int y, bool bSticky );

    void DoMoveLine();



    CZMBoxSelect* GetBoxSelect() const { return m_BoxSelect; }
    CZMLineTool* GetLineTool() const { return m_LineTool; }

private:
    ZMClickMode_t m_iClickMode;

    float m_flLastLeftClick;
    float m_flLastRightClick;


    CZMBoxSelect* m_BoxSelect;
    CZMLineTool* m_LineTool;


    bool m_bDraggingLeft;
    bool m_bDraggingRight;


    // Client mode will call some of our functions.
    friend class ClientModeZMNormal;
};

extern CZMViewBase* g_pZMView;


extern void UTIL_TraceZMView( trace_t* trace, Vector endpos, int mask, CTraceFilterSimple* filter = nullptr, C_BaseEntity* pEnt = nullptr, int collisionGroup = COLLISION_GROUP_NONE );
