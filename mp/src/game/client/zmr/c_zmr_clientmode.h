#pragma once

#include "clientmode_shared.h"


class ClientModeZMNormal : public ClientModeShared
{
public:
    DECLARE_CLASS( ClientModeZMNormal, ClientModeShared );

    ClientModeZMNormal();
    ~ClientModeZMNormal();

    virtual void Init() OVERRIDE;

    virtual bool DoPostScreenSpaceEffects( const CViewSetup* pSetup ) OVERRIDE;
    virtual void PostRender() OVERRIDE;

    virtual int KeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding );


    bool IsZMHoldingCtrl() const { return m_bZMHoldingCtrl; }
    void SetZMHoldingCtrl( bool state ) { m_bZMHoldingCtrl = state; }

private:
    int ZMKeyInput( int down, ButtonCode_t keynum, const char* pszCurrentBinding );

    bool m_bZMHoldingCtrl;
};

extern ClientModeZMNormal* GetZMClientMode();
