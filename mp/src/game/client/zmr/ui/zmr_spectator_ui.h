#pragma once


#include <spectatorgui.h>


class CZMSpectatorGUI : public CSpectatorGUI
{
private:
    DECLARE_CLASS_SIMPLE( CZMSpectatorGUI, CSpectatorGUI );

public:
    CZMSpectatorGUI( IViewPort* pViewPort );

    virtual void Update( void );
    virtual bool NeedsUpdate( void );

protected:
    int             m_nLastSpecHealth;
    int             m_nLastSpecMode;
    C_BaseEntity*   m_pLastSpecTarget;
};
