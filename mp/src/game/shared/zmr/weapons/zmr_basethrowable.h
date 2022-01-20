#pragma once


#include "zmr_weaponconfig.h"
#include "zmr_base.h"


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

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE;
    virtual bool OverrideFromConfig( KeyValues* kv ) OVERRIDE;

    virtual KeyValues* ToKeyValues() const OVERRIDE;

    float flProjectileDamage;
    float flProjectileRadius;
    
    float flThrowVelocity;

    Vector vecAngularVel_Min;
    Vector vecAngularVel_Max;
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


    virtual void Precache() OVERRIDE;

    virtual void ItemPostFrame() OVERRIDE;

    void Equip( CBaseCombatCharacter* pCharacter ) OVERRIDE;
    bool Deploy() OVERRIDE;

#ifndef CLIENT_DLL
    void Drop( const Vector& vecVelocity ) OVERRIDE;

    virtual void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) OVERRIDE;
#endif

    virtual const char* GetProjectileClassname() const;

    virtual void Throw( CZMPlayer* pPlayer );
    virtual Vector GetThrowPos( CZMPlayer* pPlayer );
    virtual Vector GetThrowDirection( CZMPlayer* pPlayer );

    float GetThrowVelocity() const;
    QAngle GetThrowAngularVelocity() const;


    ZMThrowState_t GetThrowState() const { return m_iThrowState; }
protected:
    void SetThrowState( ZMThrowState_t state ) { m_iThrowState = state; }

    virtual void PostThrow( CZMPlayer* pPlayer, CBaseEntity* pProjectile );


private:
    CNetworkVar( ZMThrowState_t, m_iThrowState );
};
