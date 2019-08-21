#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MAX_SHOTS                   16

#define NUM_BULLET_SEED_BITS        8


class CZMTEFireBullets : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CZMTEFireBullets, CBaseTempEntity );
	DECLARE_SERVERCLASS();

    CZMTEFireBullets();
	~CZMTEFireBullets();

public:
	CNetworkVar( int, m_iPlayer );
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecDir );
	CNetworkVar( int, m_iAmmoID );
	CNetworkVar( int, m_iSeed );
	CNetworkVar( int, m_iShots );
	CNetworkVar( float, m_flSpread );
	CNetworkVar( bool, m_bDoImpacts );
	CNetworkVar( bool, m_bDoTracers );
};


CZMTEFireBullets::CZMTEFireBullets() : CBaseTempEntity( "ZMR Shotgun Shot" )
{
}

CZMTEFireBullets::~CZMTEFireBullets()
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CZMTEFireBullets, DT_ZM_TE_FireBullets )
	SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecDir ), -1 ),
	SendPropInt( SENDINFO( m_iAmmoID ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSeed ), NUM_BULLET_SEED_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iShots ), Q_log2( MAX_SHOTS ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iPlayer ), Q_log2( MAX_PLAYERS ) + 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flSpread ), 10, 0, 0, 1 ),	
	SendPropBool( SENDINFO( m_bDoImpacts ) ),
	SendPropBool( SENDINFO( m_bDoTracers ) ),
END_SEND_TABLE()


static CZMTEFireBullets g_TEZMFireBullets;


void TE_ZMFireBullets(
	int	iPlayerIndex,
	const Vector &vOrigin,
	const Vector &vDir,
	int	iAmmoID,
	int iSeed,
	int iShots,
	float flSpread,
	bool bDoTracers,
	bool bDoImpacts )
{
	CPASFilter filter( vOrigin );
	filter.UsePredictionRules();

	g_TEZMFireBullets.m_iPlayer = iPlayerIndex;
	g_TEZMFireBullets.m_vecOrigin = vOrigin;
	g_TEZMFireBullets.m_vecDir = vDir;
	g_TEZMFireBullets.m_iSeed = iSeed;
	g_TEZMFireBullets.m_iShots = iShots;
	g_TEZMFireBullets.m_flSpread = flSpread;
	g_TEZMFireBullets.m_iAmmoID = iAmmoID;
	g_TEZMFireBullets.m_bDoTracers = bDoTracers;
	g_TEZMFireBullets.m_bDoImpacts = bDoImpacts;
	
	Assert( iSeed < (1 << NUM_BULLET_SEED_BITS) );
	
	g_TEZMFireBullets.Create( filter, 0 );
}
