#pragma once

#include "basemodelpanel.h"


class CZMModelPanel : public CModelPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMModelPanel, CModelPanel );

    CZMModelPanel( vgui::Panel *parent, const char *name );


    virtual void Paint() OVERRIDE;
    virtual void DeleteModelData( void ) OVERRIDE;

protected:
    virtual void SetupModel( void ) OVERRIDE;
};
