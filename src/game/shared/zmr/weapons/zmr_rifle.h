#pragma once


#include "zmr_basepump.h"


#ifdef CLIENT_DLL
#define CZMWeaponRifle C_ZMWeaponRifle
#endif

class CZMWeaponRifle : public CZMBasePumpWeapon
{
public:
    DECLARE_CLASS( CZMWeaponRifle, CZMBasePumpWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CZMWeaponRifle();
    ~CZMWeaponRifle();


    virtual Vector GetBulletSpread() const override;
    virtual bool Holster( CBaseCombatWeapon* pSwitchTo = nullptr ) override;
    virtual void Drop( const Vector& vecVelocity ) override;
    virtual void ItemBusyFrame() override;
    virtual void ItemPostFrame() override;

    virtual Activity GetIdleActivity() const override { return IsZoomed() ? ACT_VM_IDLE_SPECIAL : BaseClass::GetIdleActivity(); }
    virtual Activity GetReloadStartAct() const override { return ACT_VM_RELOAD_START; }
    virtual Activity GetReloadEndAct() const override { return ACT_VM_RELOAD_FINISH; }
    virtual Activity GetPrimaryAttackActivity() override;
    virtual Activity GetPumpAct() const override;

    virtual bool IsZoomed() const override { return m_bInZoom; }
    virtual bool CanZoom() const;

    virtual void StartReload() override;

    void CheckToggleZoom();
    void CheckUnZoom();
    void ToggleZoom();
    void Zoom( CZMPlayer* pOwner ); // ZMRTODO: Do something more reasonable...
    void UnZoom( CZMPlayer* pOwner );

protected:
    CNetworkVar( bool, m_bInZoom );
};
