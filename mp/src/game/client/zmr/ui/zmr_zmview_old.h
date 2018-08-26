#pragma once


class CZMLineTool;

#include "zmr_boxselect.h"
#include "zmr_cntrlpanel.h"
#include "zmr_buildmenu.h"
#include "zmr_buildmenu_new.h"
#include "zmr_manimenu.h"
#include "zmr_manimenu_new.h"

#include "zmr_zmview_base.h"

class CZMViewOld : public CZMViewBase

{
public:
    DECLARE_CLASS_SIMPLE( CZMViewOld, CZMViewBase );


    CZMViewOld( const char* pElementName );
    ~CZMViewOld();


    virtual CZMBuildMenuBase* GetBuildMenu() OVERRIDE;
    virtual CZMManiMenuBase* GetManiMenu() OVERRIDE;

protected:
    virtual void CloseChildMenus() OVERRIDE;

private:
    CZMHudControlPanel* m_pZMControl;
    CZMManiMenu* m_pManiMenu;
    CZMManiMenuNew* m_pManiMenuNew;
    CZMBuildMenu* m_pBuildMenu;
    CZMBuildMenuNew* m_pBuildMenuNew;
};
