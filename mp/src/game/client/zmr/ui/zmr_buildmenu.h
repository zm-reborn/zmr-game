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

class CZMBuildMenu : public vgui::Frame, public IViewPortPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenu, vgui::Frame );


    CZMBuildMenu( IViewPort* pViewPort );
    ~CZMBuildMenu();

    virtual const char* GetName() OVERRIDE { return "ZMBuildMenu"; };
    void SetData( KeyValues *data ) OVERRIDE {};
    void Reset() OVERRIDE {};
    void Update() OVERRIDE {};
    bool NeedsUpdate( void ) OVERRIDE { return false; }
    bool HasInputElements( void ) OVERRIDE { return true; }
    //void Close() OVERRIDE;
    void ShowPanel( bool state ) OVERRIDE;
	void OnThink( void );
	void OnClose();

    vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); }
    bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); }
    void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); }

    void OnCommand( const char *command ) OVERRIDE;

    //void OnFrameFocusChanged( bool ) OVERRIDE;


    IViewPort* m_pViewPort;


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
	vgui::HFont m_hMediumFont;
	vgui::HFont m_hLargeFont;
    vgui::HScheme scheme;

	//virtual void Paint();
	void CalculateButtonState();

	void AutoAssign();
	int m_iLastFlags;
	
	void UpdateQueue(const int q[], int size = BM_QUEUE_SIZE);

	void OnKeyCodePressed(vgui::KeyCode code);

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