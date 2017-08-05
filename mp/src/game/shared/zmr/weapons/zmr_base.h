#pragma once

//#include "hl2mp/hl2mp_player_shared.h"
#include "basecombatweapon_shared.h"
//#include "hl2mp/weapon_hl2mpbase.h"
//#include "hl2mp/hl2mp_weapon_parse.h"


#include "zmr/zmr_weapon_parse.h"

#include "zmr/zmr_player_shared.h"



#ifdef CLIENT_DLL
#define CZMBaseWeapon C_ZMBaseWeapon
#endif

/*
    NOTE:

    You must add custom animation events in eventlist.h && eventlist.cpp
*/

class CZMBaseWeapon : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CZMBaseWeapon, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
    DECLARE_DATADESC();

	CZMBaseWeapon();
	~CZMBaseWeapon();
    

    const CZMWeaponInfo& GetWpnData() const;
    // NOTE: Always use this to get the damage from .txt file.
    virtual void FireBullets( const FireBulletsInfo_t &info ) OVERRIDE;
    virtual void PrimaryAttack() OVERRIDE;
    

    virtual bool IsPredicted() const OVERRIDE { return true; };

#ifdef CLIENT_DLL
    virtual void OnDataChanged( DataUpdateType_t ) OVERRIDE;
    virtual bool ShouldPredict() OVERRIDE;
#endif

    void WeaponSound( WeaponSound_t, float soundtime = 0.0f ) OVERRIDE;

    void DoMachineGunKick( float, float, float, float );


    virtual int GetMinBurst( void ) OVERRIDE { return 1; };
    virtual int GetMaxBurst( void ) OVERRIDE { return 1; };
    
#ifndef CLIENT_DLL
    virtual void Materialize( void ) OVERRIDE;
#endif
    // Makes our weapons not cry about spawning.
    virtual void FallInit( void );

    // Override this so our guns don't disappear.
    virtual void SetPickupTouch( void ) OVERRIDE;

    virtual bool CanBeSelected( void ) OVERRIDE;
    // Never let anybody tell you're not beautiful even without any ammo, alright?
    // Let us always select this weapon even when we don't have any ammo for it.
    virtual bool AllowsAutoSwitchFrom( void ) const OVERRIDE { return false; };
    virtual void Drop( const Vector& ) OVERRIDE;

    // Add weapon slot flag.
    virtual void Equip( CBaseCombatCharacter* ) OVERRIDE;


    // Viewmodel/misc stuff
    float CalcViewmodelBob( void );
    void AddViewmodelBob( CBaseViewModel*, Vector&, QAngle& );
    Vector GetBulletSpread( WeaponProficiency_t );
    float GetSpreadBias( WeaponProficiency_t );
    const WeaponProficiencyInfo_t* GetProficiencyValues();
    const WeaponProficiencyInfo_t* GetDefaultProficiencyValues();

    // Our stuff
    CZMPlayer* GetPlayerOwner();
    virtual bool CanBeDropped() { return true; };


#ifndef CLIENT_DLL
    virtual const char* GetDropAmmoName() { return nullptr; };
    virtual int GetDropAmmoAmount() { return 1; };

    int GetSlotFlag() { return m_iSlotFlag; };

    void FreeWeaponSlot();
#endif

protected:
#ifndef CLIENT_DLL
    inline void SetSlotFlag( int flags ) { m_iSlotFlag = flags; };

    int m_iSlotFlag;
#endif
};