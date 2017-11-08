#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <utlvector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr_panel_world.h"
#include "zmr/zmr_shareddefs.h"



#define BM_QUEUE_SIZE       10

class CZMBuildMenuNew : public CZMWorldPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenuNew, CZMWorldPanel );


    CZMBuildMenuNew( vgui::Panel* pParent );
    ~CZMBuildMenuNew();

	void OnThink( void );
	void OnClose();


    virtual const char* GetName() OVERRIDE { return "ZMBuildMenuNew"; };


    virtual void ShowPanel( bool state ) OVERRIDE;
    virtual void SetData( KeyValues *data ) OVERRIDE {};
    virtual void Reset() OVERRIDE {};
    virtual void Update() OVERRIDE {};
    virtual bool NeedsUpdate( void ) OVERRIDE { return false; };
    virtual bool HasInputElements( void ) OVERRIDE { return true; };

    virtual void Close() OVERRIDE { OnClose(); SetVisible( false ); };
    virtual void OnCommand( const char *command ) OVERRIDE;


    inline int GetLastSpawnIndex() { return m_iLastSpawnIndex; };
    inline int GetSpawnIndex() { return m_iSpawnIndex; };
    inline void SetSpawnIndex( int entindex ) { m_iSpawnIndex = entindex; };

    inline int GetZombieFlags() { return m_fSpawnZombieFlags; };
    inline void SetZombieFlags( int flags ) { m_fSpawnZombieFlags = flags; };

    void UpdateQueue( const ZombieClass_t queue[] );

private:
    int m_iLastSpawnIndex;
    int m_iSpawnIndex;
    int m_fSpawnZombieFlags;

    vgui::ImagePanel* m_pQueue[BM_QUEUE_SIZE];
    vgui::IImage* m_pQueueImages[ZMCLASS_MAX];
};

extern CZMBuildMenuNew* g_pBuildMenuNew;