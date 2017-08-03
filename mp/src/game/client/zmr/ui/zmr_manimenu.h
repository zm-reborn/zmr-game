#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <UtlVector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr/zmr_shareddefs.h"



#define TYPE_TOTAL 5
#define BM_QUEUE_SIZE 10

class CZMManiMenu : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenu, vgui::Frame );


    CZMManiMenu( IViewPort* pViewPort );
    ~CZMManiMenu();

    virtual const char* GetName() OVERRIDE { return "ZMManiMenu"; };
    void SetData( KeyValues *data ) OVERRIDE {};
    void Reset() OVERRIDE {};
    void Update() OVERRIDE {};
    bool NeedsUpdate( void ) OVERRIDE { return false; }
    bool HasInputElements( void ) OVERRIDE { return true; }
    //void Close() OVERRIDE;
    void ShowPanel( bool state ) OVERRIDE;
	void OnThink( void );
	//void OnClose();

    vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); }
    bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); }
    void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); }

    void OnCommand( const char *command ) OVERRIDE;


    IViewPort* m_pViewPort;


    inline int GetTrapIndex() { return m_iTrapIndex; };
    inline void SetTrapIndex( int index ) { m_iTrapIndex = index; };
    void SetDescription( const char* pszIn );

    inline int GetTrapCost() { return m_nTrapCost; };
    void SetTrapCost( int );
    inline int GetCost() { return m_nCost; };
    void SetCost( int );

private:
    int m_iTrapIndex;
    int m_nCost;
    int m_nTrapCost;


    vgui::HFont m_hMediumFont;
	vgui::HFont m_hLargeFont;
	vgui::HScheme	scheme;
};

extern CZMManiMenu* g_pManiMenu;