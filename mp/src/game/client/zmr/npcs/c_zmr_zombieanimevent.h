#pragma once

#include "c_basetempentity.h"

#include "c_zmr_zombiebase.h"


class C_ZMTEZombieAnimEvent : public C_BaseTempEntity
{
public:
    DECLARE_CLASS( C_ZMTEZombieAnimEvent, C_BaseTempEntity );
    DECLARE_CLIENTCLASS();

    virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;

public:
    CNetworkHandle( C_ZMBaseZombie, m_hZombie );
    CNetworkVar( int, m_iEvent );
    CNetworkVar( int, m_nData );
};
