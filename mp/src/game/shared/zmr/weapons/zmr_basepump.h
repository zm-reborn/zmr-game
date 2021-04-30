#pragma once


#include "zmr_base.h"


#ifdef CLIENT_DLL
#define CZMBasePumpWeapon C_ZMBasePumpWeapon
#endif


enum PumpState_t
{
    PUMPSTATE_NONE = 0,

    PUMPSTATE_PUMP_EJECT, // Shell needs to be ejected
    PUMPSTATE_PUMP_EMPTY, // Nothing in chamber.

    PUMPSTATE_MAX
};

enum ReloadState_t
{
    RELOADSTATE_NONE = 0, // Not in reload.

    RELOADSTATE_START, // We're starting to reload. (transition to looping anim)
    RELOADSTATE_RELOADING, // Currently in the reload animation.

    // There is no finish state, as we have no use for it.

    RELOADSTATE_MAX
};

class CZMBasePumpConfig : public ZMWeaponConfig::CZMBaseWeaponConfig
{
public:
    CZMBasePumpConfig( const char* wepname, const char* configpath );

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE;

    virtual KeyValues* ToKeyValues() const OVERRIDE;

    
    float flPumpTime;
};


// ZMRTODO: Separate pump and single reload weapon classes.
class CZMBasePumpWeapon : public CZMBaseWeapon
{
public:
    DECLARE_CLASS( CZMBasePumpWeapon, CZMBaseWeapon );
    DECLARE_NETWORKCLASS(); 
    DECLARE_PREDICTABLE();


    CZMBasePumpWeapon();

    const CZMBasePumpConfig* GetBasePumpConfig() const;

    virtual bool CanAct( ZMWepActionType_t type = WEPACTION_GENERIC ) const OVERRIDE; 

    virtual void Drop( const Vector& vecVelocity ) OVERRIDE;
    virtual bool Holster( CBaseCombatWeapon* pSwitchTo ) OVERRIDE;
    virtual void Shoot( int iAmmoType = -1, int nBullets = -1, int nAmmo = -1, float flMaxRange = -1.0f, bool bSecondary = false ) OVERRIDE;
    virtual	void CheckReload() OVERRIDE;
    virtual bool Reload() OVERRIDE;
    virtual void ItemPostFrame() OVERRIDE;
    virtual void FinishReload() OVERRIDE;

    virtual Activity GetReloadStartAct() const { return ACT_VM_RELOAD_START; }
    virtual Activity GetReloadEndAct() const { return ACT_VM_RELOAD_FINISH; }
    virtual Activity GetPumpAct() const { return ACT_SHOTGUN_PUMP; }
    virtual Activity GetEmptyPumpAct() const { return ACT_VM_RELOAD_SILENCED; }
    virtual void StartReload();
    virtual void Pump();

    virtual void StopReload() OVERRIDE;
    virtual void CancelReload() OVERRIDE;
    virtual bool ShouldCancelReload() const OVERRIDE;

    virtual bool IsInReload() const OVERRIDE;
    bool NeedsPump() const { return m_iPumpState != PUMPSTATE_NONE; }
    
protected:
    CNetworkVar( int, m_iPumpState );
    CNetworkVar( int, m_iReloadState );
    CNetworkVar( bool, m_bCancelReload );
};
