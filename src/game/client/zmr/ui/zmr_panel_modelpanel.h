#pragma once

#include "basemodelpanel.h"


class CZMModelPanel : public CModelPanel
{
public:
    DECLARE_CLASS_SIMPLE( CZMModelPanel, CModelPanel );

    CZMModelPanel( vgui::Panel *parent, const char *name );


    virtual void Paint() override;
    virtual void DeleteModelData( void ) override;

protected:
    virtual void SetupModel( void ) override;
};
