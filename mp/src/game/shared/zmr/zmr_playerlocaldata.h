#pragma once


#include "networkvar.h"

#include "hl_movedata.h"

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
    CNetworkVar( float, m_flAccuracyRatio );
    //float m_flAccuracyRatio;


	// Ladder related data
	CNetworkVar( EHANDLE, m_hLadder );
	LadderMove_t m_LadderMove;
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ZM_PlyLocal );
#else
EXTERN_SEND_TABLE( DT_ZM_PlyLocal );
#endif
