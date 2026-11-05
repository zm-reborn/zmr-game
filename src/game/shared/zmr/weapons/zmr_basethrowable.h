#pragma once


#include "zmr_weaponconfig.h"
#include "zmr_base.h"

#ifndef CLIENT_DLL
#include "zmr_baseprojectile.h"
#endif

#ifdef CLIENT_DLL
#define CZMBaseThrowableWeapon C_ZMBaseThrowableWeapon
#endif


enum ZMThrowState_t
{
    THROWSTATE_IDLE = 0,
    
    THROWSTATE_ARMING, // Igniting. DRAW_BACK needs to be called manually...

    THROWSTATE_DRAW_BACK, // Getting it ready to throw.

    THROWSTATE_READYTOTHROW, // Throwable!

    THROWSTATE_THROWN, // Thrown. Remove me.


    MOLOTOVSTATE_MAX,
};

class CZMBaseThrowableConfig : public ZMWeaponConfig::CZMBaseWeaponConfig
{
public:
    CZMBaseThrowableConfig( const char* wepname, const char* configpath );

    virtual void LoadFromConfig( KeyValues* kv ) override;
    virtual bool OverrideFromConfig( KeyValues* kv ) override;

    virtual KeyValues* ToKeyValues() const override;

    float flProjectileDamage;
    float flProjectileRadius;
    
    float flThrowVelocity;

    Vector vecAngularVel_Min;
    Vector vecAngularVel_Max;

    // Cook the projectile?
    bool bArmOnReady;

    // How long until detonating after arming.
    float flDetonationTime;
};


class CZMBaseThrowableWeapon : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMBaseThrowableWeapon, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMBaseThrowableWeapon();
    ~CZMBaseThrowableWeapon();


    const CZMBaseThrowableConfig* GetBaseThrowableConfig() const;


    virtual void Precache() override;

    virtual void ItemPostFrame() override;

    virtual bool CanHolster() const override { return GetThrowState() <= THROWSTATE_ARMING; }
    virtual bool CanBeDropped() const override { return CanHolster(); }

    virtual void Equip( CBaseCombatCharacter* pCharacter ) override;
    virtual bool Deploy() override;

#ifndef CLIENT_DLL
    void Drop( const Vector& vecVelocity ) override;

    virtual void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) override;
#endif

    virtual const char* GetProjectileClassname() const;

    virtual void Throw( CZMPlayer* pPlayer );
    virtual Vector GetThrowPos( CZMPlayer* pPlayer );
    virtual Vector GetThrowDirection( CZMPlayer* pPlayer );

    float GetThrowVelocity() const;
    QAngle GetThrowAngularVelocity() const;


    ZMThrowState_t GetThrowState() const { return m_iThrowState; }

#ifndef CLIENT_DLL
    void OnProjectileExplode();
#endif

protected:
    void SetThrowState( ZMThrowState_t state ) { m_iThrowState = state; }

    virtual void PostThrow( CZMPlayer* pPlayer, CBaseEntity* pProjectile ) {}

#ifndef CLIENT_DLL
    CZMBaseProjectile* CreateProjectile() const;
    CZMBaseProjectile* GetCookedProjectile() const { return m_hCookedProjectile.Get(); }
    void PostThrowCleanup();
#endif

private:
    CNetworkVar( ZMThrowState_t, m_iThrowState );

#ifndef CLIENT_DLL
    void TryArmOnReady();
    void UnfollowCookedProjectile();
    void DropCookedProjectile();

    CHandle<CZMBaseProjectile> m_hCookedProjectile;
#endif
};
