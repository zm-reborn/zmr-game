#pragma once

#include "zmr_menu_world.h"
#include "zmr_manimenu_base.h"
#include "zmr_shareddefs.h"


class CZMRadialPanel;

class CZMManiMenuNew : public CZMManiMenuBase, public CZMWorldMenu
{
public:
    DECLARE_CLASS_SIMPLE( CZMManiMenuNew, CZMManiMenuBase );


    CZMManiMenuNew( vgui::Panel* pParent );
    ~CZMManiMenuNew();

    virtual const char* GetName() override { return "ZMManiMenu"; };

    virtual void Paint() override;

    virtual void ShowPanel( bool state ) override;
	virtual void OnThink( void ) override;


    virtual void ShowMenu( C_ZMEntManipulate* pMani ) override;
    virtual void SetDescription( const char* ) override;
    virtual void SetCost( int ) override;
    virtual void SetTrapCost( int ) override;

private:
    CZMRadialPanel* m_pRadial;
    vgui::Label* m_pDescLabel;
};
