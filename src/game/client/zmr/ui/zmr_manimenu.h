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

    virtual const char* GetName() override { return "ZMManiMenu"; };

    virtual void ShowPanel( bool state ) override;
	virtual void OnThink( void ) override;



    


    virtual void SetDescription( const char* ) override;
    virtual void SetCost( int ) override;
    virtual void SetTrapCost( int ) override;
};
