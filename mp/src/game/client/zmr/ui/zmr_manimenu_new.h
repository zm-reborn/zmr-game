#pragma once

#include "zmr_menu_world.h"
#include "zmr_manimenu_base.h"
#include "zmr/zmr_shareddefs.h"


class CZMRadialPanel;

class CZMManiMenuNew : public CZMManiMenuBase, public CZMWorldMenu
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenuNew, CZMManiMenuBase );


    CZMManiMenuNew( vgui::Panel* pParent );
    ~CZMManiMenuNew();

    virtual const char* GetName() OVERRIDE { return "ZMManiMenu"; };

    virtual void Paint() OVERRIDE;

    virtual void ShowPanel( bool state ) OVERRIDE;
	virtual void OnThink( void ) OVERRIDE;


    virtual void ShowMenu( C_ZMEntManipulate* pMani ) OVERRIDE;
    virtual void SetDescription( const char* ) OVERRIDE;
    virtual void SetCost( int ) OVERRIDE;
    virtual void SetTrapCost( int ) OVERRIDE;

private:
    CZMRadialPanel* m_pRadial;
    vgui::Label* m_pDescLabel;
};
