#pragma once

#include "cbase.h"

#include "cdll_util.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include <vgui_controls/Frame.h>


#include "zmr/c_zmr_entities.h"


struct ZMQueueSlotData_t
{
    uint8 nCount;
    ZombieClass_t zclass;
};

abstract_class CZMBuildMenuBase : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenuBase, vgui::Frame );


    CZMBuildMenuBase( vgui::Panel* pParent, const char* name );
    ~CZMBuildMenuBase();


    virtual vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); };
    virtual bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); };
    virtual void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); };

    virtual void SetData( KeyValues *data ) OVERRIDE {};
    virtual void Reset() OVERRIDE {};
    virtual void Update() OVERRIDE {};
    virtual bool NeedsUpdate( void ) OVERRIDE { return false; }
    virtual bool HasInputElements( void ) OVERRIDE { return true; }

    virtual void OnCommand( const char* command ) OVERRIDE;
    virtual void OnClose() OVERRIDE;


    virtual int ZMKeyInput( ButtonCode_t keynum, int down );

    virtual void ShowMenu( C_ZMEntZombieSpawn* pSpawn );
    virtual void UpdateQueue( const ZMQueueSlotData_t q[], int size ) {};

    inline int GetLastSpawnIndex() { return m_iLastSpawnIndex; };
    inline int GetSpawnIndex() { return m_iSpawnIndex; };
    inline void SetSpawnIndex( int entindex ) { m_iSpawnIndex = entindex; };

    inline int GetZombieFlags() { return m_fSpawnZombieFlags; };
    inline void SetZombieFlags( int flags ) { m_fSpawnZombieFlags = flags; };

    int GetAltSpawnAmount() const;

private:
    int m_iLastSpawnIndex;
    int m_iSpawnIndex;
    int m_fSpawnZombieFlags;
};
