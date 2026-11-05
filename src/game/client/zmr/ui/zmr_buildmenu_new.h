#pragma once

#include "baseviewport.h"
#include "hudelement.h"


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr_menu_world.h"
#include "zmr_buildmenu_base.h"
#include "zmr_shareddefs.h"



class CZMRadialButton;
class CZMRadialPanel;
class CZMImageRow;

class CZMBuildMenuNew : public CZMBuildMenuBase, public CZMWorldMenu
{
public:
    DECLARE_CLASS_SIMPLE( CZMBuildMenuNew, CZMBuildMenuBase );


    CZMBuildMenuNew( vgui::Panel* pParent );
    ~CZMBuildMenuNew();

    virtual void Paint() override;

    virtual void ShowPanel( bool state ) override;
	virtual void OnThink() override;


    
    MESSAGE_FUNC_PARAMS( OnImageRowPressed, "OnImageRowPressed", kv );
    MESSAGE_FUNC_PARAMS( OnRadialOver, "OnRadialOver", kv );
    MESSAGE_FUNC_PARAMS( OnRadialLeave, "OnRadialLeave", kv );

    virtual const char* GetName() override { return "ZMBuildMenuNew"; };

    virtual void ShowMenu( C_ZMEntZombieSpawn* pSpawn ) override;
    virtual void UpdateMenu( C_ZMEntZombieSpawn* pSpawn ) override;
    virtual void UpdateQueue( const ZMQueueSlotData_t queue[], int size ) override;

private:
    void UpdateButtons();

    void UpdateButtonData( CZMRadialButton* pButton );

    CZMRadialPanel* m_pRadial;
    CZMImageRow* m_pImageList;
    int m_iCurTooltip;

    vgui::IImage* m_pQueueImages[ZMCLASS_MAX];
};
