#pragma once

#include "items.h"

class CZMAmmo : public CItem
{
public:
    DECLARE_CLASS( CZMAmmo, CItem );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    CZMAmmo();


    virtual void        Spawn() OVERRIDE;
    virtual void        Precache() OVERRIDE;

    virtual bool        MyTouch( CBasePlayer* pPlayer ) OVERRIDE;


    void                SetNextPickupTouch( float delay = 1.0f );

protected:
    void                NoPickupThink();
    void                EmptyTouch( CBaseEntity* pOther );

    virtual const char* GetItemModel() { return nullptr; };
    virtual const char* GetAmmoName() { return nullptr; };
    virtual int         GetAmmoCount() { return 0; };


    CNetworkVar( int, m_iAmmoType );
};
