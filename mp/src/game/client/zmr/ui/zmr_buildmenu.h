#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <utlvector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr/zmr_shareddefs.h"



#define TYPE_TOTAL 5
#define BM_QUEUE_SIZE 10

class CZMBuildMenu : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenu, vgui::Frame );


    CZMBuildMenu( vgui::Panel* pParent );
    ~CZMBuildMenu();

	void OnThink( void );
	void OnClose();


    virtual const char* GetName() OVERRIDE { return "ZMBuildMenu"; };


    virtual void ShowPanel( bool state ) OVERRIDE;
    virtual void SetData( KeyValues *data ) OVERRIDE {};
    virtual void Reset() OVERRIDE {};
    virtual void Update() OVERRIDE {};
    virtual bool NeedsUpdate( void ) OVERRIDE { return false; };
    virtual bool HasInputElements( void ) OVERRIDE { return true; };
    virtual vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); };
    virtual bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); };
    virtual void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); };

    void OnCommand( const char *command ) OVERRIDE;


    inline int GetLastSpawnIndex() { return m_iLastSpawnIndex; };
    inline int GetSpawnIndex() { return m_iSpawnIndex; };
    inline void SetSpawnIndex( int entindex ) { m_iSpawnIndex = entindex; };

    inline int GetZombieFlags() { return m_fSpawnZombieFlags; };
    inline void SetZombieFlags( int flags ) { m_fSpawnZombieFlags = flags; };

private:
    int m_iLastSpawnIndex;
    int m_iSpawnIndex;
    int m_fSpawnZombieFlags;

public:

	void CalculateButtonState();

	void AutoAssign();
	int m_iLastFlags;
	
	void UpdateQueue(const int q[], int size = BM_QUEUE_SIZE);

protected:


	//int             GetCostForType(int type) const;

	void			ShowZombieInfo(int type);


	// helper functions
	void			SetLabelText(const char *textEntryName, const char *text);
	void			GetLabelText(const char* element, char *buffer, int buflen);
	
	// command callbacks
	

	

	int				m_iJumpKey;
	//int				m_iScoreBoardKey;

	vgui::Panel		*spawnbuttons[TYPE_TOTAL];
	vgui::Panel		*spawnfives[TYPE_TOTAL];
	vgui::IImage	*zombieimages[TYPE_TOTAL];
	vgui::IImage	*zombiequeue[TYPE_TOTAL];
	char*			zombiedescriptions[TYPE_TOTAL];

	//unit info area
	vgui::ImagePanel *info_image;
	vgui::Label		*info_rescost;
	vgui::Label		*info_popcost;
	vgui::Label		*info_description;

	//queue
	//CUtlLinkedList<vgui::ImagePanel*> queueimages; //we'll be allocating these
	vgui::ImagePanel *queueimages[BM_QUEUE_SIZE];

	vgui::Button	*removelast;
	vgui::Button	*clearqueue;
};

extern CZMBuildMenu* g_pBuildMenu;