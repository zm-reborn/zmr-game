#pragma once


#include "zmr_base.h"


#ifdef CLIENT_DLL
#define CZMBasePumpWeapon C_ZMBasePumpWeapon
#endif


#define RELOADSTATE_NONE        0
#define RELOADSTATE_START       1
#define RELOADSTATE_RELOADING   2

// ZMRTODO: Separate pump and single reload weapon classes.
class CZMBasePumpWeapon : public CZMBaseWeapon
{
public:
    DECLARE_CLASS( CZMBasePumpWeapon, CZMBaseWeapon );
    DECLARE_NETWORKCLASS(); 
    DECLARE_PREDICTABLE();


    CZMBasePumpWeapon();


    virtual bool Holster( CBaseCombatWeapon* pSwitchTo ) OVERRIDE;
    virtual void PrimaryAttack() OVERRIDE;
    virtual	void CheckReload() OVERRIDE;
    virtual bool Reload() OVERRIDE;
    virtual void ItemPostFrame() OVERRIDE;
    virtual void FinishReload() OVERRIDE;

    virtual Activity GetReloadStartAct() { return ACT_VM_RELOAD_START; }
    virtual Activity GetReloadEndAct() { return ACT_VM_RELOAD_FINISH; }
    virtual Activity GetPumpAct() { return ACT_SHOTGUN_PUMP; }
    virtual void StartReload();
    virtual void Pump();

    virtual void StopReload() OVERRIDE;
    virtual void CancelReload() OVERRIDE;
    virtual bool ShouldCancelReload() const OVERRIDE;

    virtual bool CanPickupAmmo() const OVERRIDE { return !IsInReload(); }
    virtual bool IsInReload() const OVERRIDE;
    
protected:
    CNetworkVar( bool, m_bNeedPump );
    CNetworkVar( int, m_iReloadState );
    CNetworkVar( bool, m_bCancelReload );
};
