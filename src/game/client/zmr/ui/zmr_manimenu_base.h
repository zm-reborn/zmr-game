#pragma once

#include "cbase.h"

#include "cdll_util.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include <vgui_controls/Frame.h>


#include "c_zmr_entities.h"


abstract_class CZMManiMenuBase : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenuBase, vgui::Frame );


    CZMManiMenuBase( const char* name ) : Frame( g_pClientMode->GetViewport(), name )
    {
        m_nCost = 0;
        m_nTrapCost = 0;
        m_iTrapIndex = -1;
    }
    ~CZMManiMenuBase()
    {
    }


    virtual vgui::VPANEL GetVPanel( void ) override { return BaseClass::GetVPanel(); };
    virtual bool IsVisible() override { return BaseClass::IsVisible(); };
    virtual void SetParent( vgui::VPANEL parent ) override { BaseClass::SetParent( parent ); };

    virtual void SetData( KeyValues *data ) override {};
    virtual void Reset() override {};
    virtual void Update() override {};
    virtual bool NeedsUpdate( void ) override { return false; }
    virtual bool HasInputElements( void ) override { return true; }



    virtual void OnCommand( const char *command ) override;
    virtual void OnClose() override;



    virtual void ShowMenu( C_ZMEntManipulate* pMani );
	virtual void UpdateMenu( C_ZMEntManipulate* pSpawn );
    virtual void SetDescription( const char* ) = 0;

    inline int GetTrapIndex() const { return m_iTrapIndex; };
    inline void SetTrapIndex( int index ) { m_iTrapIndex = index; };

    inline int GetCost() const { return m_nCost; };
    virtual void SetCost( int ) = 0;
    inline int GetTrapCost() const { return m_nTrapCost; };
    virtual void SetTrapCost( int ) = 0;

    virtual GameActionSet_t GetPreferredActionSet() override { return GAME_ACTION_SET_NONE; };

protected:
    int m_nCost;
    int m_nTrapCost;
    int m_iTrapIndex;
};
