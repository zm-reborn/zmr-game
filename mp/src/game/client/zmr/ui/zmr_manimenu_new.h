#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <utlvector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>


#include "zmr_menu_world.h"
#include "zmr_manimenu_base.h"
#include "zmr/zmr_shareddefs.h"



class CZMManiMenuNew : public CZMManiMenuBase, public CZMWorldMenu
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenuNew, CZMManiMenuBase );


    CZMManiMenuNew( vgui::Panel* pParent );
    ~CZMManiMenuNew();

    virtual const char* GetName() OVERRIDE { return "ZMManiMenu"; };

    virtual void ShowPanel( bool state ) OVERRIDE;
	virtual void OnThink( void ) OVERRIDE;



    virtual void ShowMenu( C_ZMEntManipulate* pMani ) OVERRIDE;
    virtual void SetDescription( const char* ) OVERRIDE;
    virtual void SetCost( int ) OVERRIDE;
    virtual void SetTrapCost( int ) OVERRIDE;
};
