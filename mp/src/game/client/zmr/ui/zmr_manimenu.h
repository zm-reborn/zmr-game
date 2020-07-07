#pragma once

#include "baseviewport.h"
#include "hudelement.h"

#include <vgui/KeyCode.h>
#include <utlvector.h>


#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>

#include "zmr_manimenu_base.h"
#include "zmr_shareddefs.h"


class CZMManiMenu : public CZMManiMenuBase
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenu, CZMManiMenuBase );


    CZMManiMenu( vgui::Panel* pParent );
    ~CZMManiMenu();

    virtual const char* GetName() OVERRIDE { return "ZMManiMenu"; };

    virtual void ShowPanel( bool state ) OVERRIDE;
	virtual void OnThink( void ) OVERRIDE;



    


    virtual void SetDescription( const char* ) OVERRIDE;
    virtual void SetCost( int ) OVERRIDE;
    virtual void SetTrapCost( int ) OVERRIDE;
};
