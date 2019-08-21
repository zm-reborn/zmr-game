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
    virtual void        OnEntityEvent( EntityEvent_t event, void* pEventData ) OVERRIDE;

    virtual bool        MyTouch( CBasePlayer* pPlayer ) OVERRIDE;


    void                SetNextPickupTouch( float delay = 1.0f );


    virtual const char* GetItemModel() const { return nullptr; }
    virtual const char* GetAmmoName() const { return nullptr; }
    int GetAmmoCount() const { return m_nAmmo; }
    int GetMaxAmmoCount() const { return m_nMaxAmmo; }

protected:
    void                NoPickupThink();
    void                EmptyTouch( CBaseEntity* pOther );


    CNetworkVar( int, m_iAmmoType );
    CNetworkVar( int, m_nAmmo );
    CNetworkVar( int, m_nMaxAmmo );

    bool m_bInThrow;
};
