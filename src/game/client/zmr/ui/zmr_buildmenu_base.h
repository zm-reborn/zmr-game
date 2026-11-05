#pragma once

#include "cbase.h"

#include "cdll_util.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include <vgui_controls/Frame.h>


#include "c_zmr_entities.h"


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


    virtual vgui::VPANEL GetVPanel( void ) override { return BaseClass::GetVPanel(); };
    virtual bool IsVisible() override { return BaseClass::IsVisible(); };
    virtual void SetParent( vgui::VPANEL parent ) override { BaseClass::SetParent( parent ); };

    virtual void SetData( KeyValues *data ) override {};
    virtual void Reset() override {};
    virtual void Update() override {};
    virtual bool NeedsUpdate( void ) override { return false; }
    virtual bool HasInputElements( void ) override { return true; }

    virtual void OnCommand( const char* command ) override;
    virtual void OnClose() override;


    virtual int ZMKeyInput( ButtonCode_t keynum, int down );

    virtual void ShowMenu( C_ZMEntZombieSpawn* pSpawn );
	virtual void UpdateMenu( C_ZMEntZombieSpawn* pSpawn );
    virtual void UpdateQueue( const ZMQueueSlotData_t q[], int size ) {};

    inline int GetLastSpawnIndex() const { return m_iLastSpawnIndex; };
    inline int GetSpawnIndex() const { return m_iSpawnIndex; };
    inline void SetSpawnIndex( int entindex ) { m_iSpawnIndex = entindex; };

    inline int GetZombieFlags() const { return m_fSpawnZombieFlags; };
    inline void SetZombieFlags( int flags ) { m_fSpawnZombieFlags = flags; };

    inline int* GetZombieCosts() const { return (int*)m_iZombieCosts; };
    void SetZombieCosts( const int *costs );

    int GetAltSpawnAmount() const;

    virtual GameActionSet_t GetPreferredActionSet() override { return GAME_ACTION_SET_NONE; };

private:
    int m_iLastSpawnIndex;
    int m_iSpawnIndex;
    int m_fSpawnZombieFlags;
    int m_iZombieCosts[ZMCLASS_MAX];
};
