#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <utlvector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr_buildmenu_base.h"
#include "zmr/c_zmr_entities.h"
#include "zmr/zmr_shareddefs.h"



#define TYPE_TOTAL 5
#define BM_QUEUE_SIZE 10

class CZMBuildMenu : public CZMBuildMenuBase
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenu, CZMBuildMenuBase );


    CZMBuildMenu( vgui::Panel* pParent );
    ~CZMBuildMenu();


    virtual void ShowPanel( bool state ) OVERRIDE;
	void OnThink( void );

    virtual const char* GetName() OVERRIDE { return "ZMBuildMenu"; };

    


	void CalculateButtonState();

	void AutoAssign();
	int m_iLastFlags;
	
	virtual void UpdateQueue( const int q[], int size ) OVERRIDE;

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
