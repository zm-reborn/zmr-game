#pragma once



#include "zmr_boxselect.h"
#include "zmr_cntrlpanel.h"
#include "zmr_buildmenu.h"
#include "zmr_manimenu.h"


enum ZMClickMode_t
{
    ZMCLICKMODE_NORMAL = 0,
    ZMCLICKMODE_TRAP,
    ZMCLICKMODE_RALLYPOINT,
    ZMCLICKMODE_HIDDEN,
    ZMCLICKMODE_PHYSEXP,

    ZMCLICKMODE_MAX
};

class CZMFrame : public CHudElement, public vgui::Frame

{
public:
    DECLARE_CLASS_SIMPLE( CZMFrame, vgui::Frame );


    CZMFrame( const char* pElementName );
    ~CZMFrame();

    virtual void Init() OVERRIDE;
    virtual void VidInit() OVERRIDE;
    virtual void Reset() OVERRIDE;
    //virtual bool ShouldDraw() OVERRIDE;
    virtual void OnMouseReleased( MouseCode code ) OVERRIDE;
    //virtual void OnMouseMoved( MouseCode code ) OVERRIDE;
    virtual void OnCursorMoved( int, int ) OVERRIDE;
    virtual void OnMousePressed( MouseCode code ) OVERRIDE;
    //virtual void OnMouseDoublePressed( MouseCode code ) OVERRIDE;
    bool ShouldDraw() OVERRIDE { return IsVisible(); };
    virtual void OnThink() OVERRIDE;
    virtual void Paint() OVERRIDE {};
    virtual void SetVisible( bool ) OVERRIDE;

    virtual void OnCommand( const char* ) OVERRIDE;
    //virtual void ShowPanel( const char* name ) OVERRIDE;
    //virtual void ShowPanel( bool state ) OVERRIDE;

    void ShowPanel( bool state ) { if ( IsVisible() == state ) return; SetVisible( state ); };
    
    inline ZMClickMode_t GetClickMode() { return m_iClickMode; };
    void SetClickMode( ZMClickMode_t, bool = false );

    static bool WorldToScreen( const Vector&, Vector&, int&, int& );

private:
    void TraceScreenToWorld( int, int, trace_t*, CTraceFilterSimple*, int );
    
    void OnLeftClick();
    void OnRightClick();

    void OnLeftRelease();
    void OnRightRelease();

    void FindZombiesInBox( int, int, int, int, bool );
    void FindZMObject( int, int, bool );

    void CloseChildMenus();

    CZMBoxSelect* m_BoxSelect;
    CZMControlPanel* m_pZMControl;
    CZMManiMenu* m_pManiMenu;
    CZMBuildMenu* m_pBuildMenu;

    MouseCode m_MouseDragStatus;

    ZMClickMode_t m_iClickMode;
};

extern CZMFrame* g_pZMView;