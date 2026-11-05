#pragma once

#include "zmr_base.h"


#ifdef CLIENT_DLL
#define CZMWeaponRevolver C_ZMWeaponRevolver
#endif

class CZMWeaponRevolver : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponRevolver, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();


    CZMWeaponRevolver();


    virtual bool Deploy() override;


    virtual Vector GetBulletSpread() const override;
    
    virtual void AddViewKick() override;

    virtual void PrimaryAttack() override;
    virtual void PrimaryAttackEffects( WeaponSound_t wpnsound ) override;
    virtual void SecondaryAttack() override;

    virtual void ItemPostFrame() override;

    void HandleAnimEventRevolver();

#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) override;
#else
    bool OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options ) override;
#endif

protected:
    CNetworkVar( float, m_flShootTime );
};
