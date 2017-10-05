#pragma once


#include "networkvar.h"

#ifdef CLIENT_DLL
#define CZMPlayerLocalData C_ZMPlayerLocalData
#endif

class CZMPlayerLocalData
{
public:
    DECLARE_CLASS_NOBASE( CZMPlayerLocalData );
    DECLARE_EMBEDDED_NETWORKVAR();

    CZMPlayerLocalData();


    CNetworkVar( int, m_fWeaponSlotFlags );
    CNetworkVar( int, m_nResources );
    CNetworkVar( float, m_flFlashlightBattery );
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ZM_PlyLocal );
#else
EXTERN_SEND_TABLE( DT_ZM_PlyLocal );
#endif
