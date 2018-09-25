#pragma once

#include "cbase.h"

#include "cdll_util.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include <vgui_controls/Frame.h>


#include "zmr/c_zmr_entities.h"


abstract_class CZMManiMenuBase : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenuBase, vgui::Frame );


    CZMManiMenuBase( const char* name ) : Frame( g_pClientMode->GetViewport(), name )
    {
    }
    ~CZMManiMenuBase()
    {
    }


    virtual vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); };
    virtual bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); };
    virtual void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); };

    virtual void SetData( KeyValues *data ) OVERRIDE {};
    virtual void Reset() OVERRIDE {};
    virtual void Update() OVERRIDE {};
    virtual bool NeedsUpdate( void ) OVERRIDE { return false; }
    virtual bool HasInputElements( void ) OVERRIDE { return true; }


    virtual void Paint() OVERRIDE;


    virtual void OnCommand( const char *command ) OVERRIDE;



    virtual void ShowMenu( C_ZMEntManipulate* pMani );
    virtual void SetDescription( const char* ) = 0;

    inline int GetTrapIndex() { return m_iTrapIndex; };
    inline void SetTrapIndex( int index ) { m_iTrapIndex = index; };

    inline int GetCost() { return m_nCost; };
    virtual void SetCost( int ) = 0;
    inline int GetTrapCost() { return m_nTrapCost; };
    virtual void SetTrapCost( int ) = 0;

protected:
    int m_nCost;
    int m_nTrapCost;
    int m_iTrapIndex;
};
