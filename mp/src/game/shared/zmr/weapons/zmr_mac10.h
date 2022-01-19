#pragma once

#include "zmr_base.h"
#include "zmr_weaponconfig.h"

#ifdef CLIENT_DLL
#define CZMWeaponMac10 C_ZMWeaponMac10
#endif


class CZMMac10Config : public ZMWeaponConfig::CZMBaseWeaponConfig
{
public:
    CZMMac10Config( const char* wepname, const char* configpath );

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE;

    virtual bool OverrideFromConfig( KeyValues* kv ) OVERRIDE;

    virtual KeyValues* ToKeyValues() const OVERRIDE;

    
    float flEasyDampen;
    float flVerticalKick;
    float flSlideLimit;
};

class CZMWeaponMac10 : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponMac10, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponMac10();


    const CZMMac10Config* GetMac10Config() const;

    
    virtual void AddViewKick() OVERRIDE;
};
