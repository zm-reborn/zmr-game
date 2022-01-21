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


    virtual bool Deploy() OVERRIDE;


    virtual Vector GetBulletSpread() const OVERRIDE;
    
    virtual void AddViewKick() OVERRIDE;

    virtual void PrimaryAttack() OVERRIDE;
    virtual void PrimaryAttackEffects( WeaponSound_t wpnsound ) OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;

    virtual void ItemPostFrame() OVERRIDE;

    void HandleAnimEventRevolver();

#ifndef CLIENT_DLL
    void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options ) OVERRIDE;
#endif

protected:
    CNetworkVar( float, m_flShootTime );
};
