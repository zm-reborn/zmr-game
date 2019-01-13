#pragma once

//#include "hl2mp/hl2mp_player_shared.h"
#include "basecombatweapon_shared.h"
//#include "hl2mp/weapon_hl2mpbase.h"
//#include "hl2mp/hl2mp_weapon_parse.h"

#ifdef CLIENT_DLL
#include "zmr/c_zmr_crosshair.h"
#endif
#include "zmr/zmr_weapon_parse.h"
#include "zmr_usercmdvalid.h"

#include "zmr/zmr_player_shared.h"



#ifdef CLIENT_DLL
#define CZMBaseWeapon C_ZMBaseWeapon
#endif

/*
    NOTE:

    You must add custom animation events in eventlist.h && eventlist.cpp
*/

// 1 is constrained.
#define SF_ZMWEAPON_TEMPLATE        ( 1 << 1 )

class CZMBaseWeapon : public CBaseCombatWeapon, public CZMUserCmdHitWepValidator
{
public:
	DECLARE_CLASS( CZMBaseWeapon, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
    DECLARE_DATADESC();
#endif

	CZMBaseWeapon();
	~CZMBaseWeapon();
    
#ifdef CLIENT_DLL
    virtual void Spawn() OVERRIDE;
#else
    virtual void Precache() OVERRIDE;
#endif
    virtual bool Reload() OVERRIDE;
    // NOTE: Always use this to get the damage from .txt file.
    virtual void FireBullets( const FireBulletsInfo_t &info ) OVERRIDE;
    virtual void FireBullets( int numShots, int iAmmoType, float flMaxDist );
    virtual void PrimaryAttack() OVERRIDE;
    virtual void Shoot();
    virtual void PrimaryAttackEffects();
    virtual void SecondaryAttack() OVERRIDE;
    
    const CZMWeaponInfo& GetWpnData() const;

    virtual int         GetMaxClip1() const OVERRIDE;
	virtual const char* GetViewModel( int vmIndex = 0 ) const OVERRIDE;
	virtual const char* GetWorldModel() const OVERRIDE;
    virtual void        SetViewModel() OVERRIDE;

#ifdef CLIENT_DLL
    virtual void    ClientThink() OVERRIDE;
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) OVERRIDE;
    void            UpdateGlow();

    virtual bool    GlowOccluded() OVERRIDE { return false; };
    virtual bool    GlowUnoccluded() OVERRIDE { return true; };


    virtual void OnDataChanged( DataUpdateType_t ) OVERRIDE;
    virtual bool ShouldPredict() OVERRIDE;
#endif
    virtual bool IsPredicted() const OVERRIDE { return true; };

    virtual void SetWeaponVisible( bool visible ) OVERRIDE;

    void WeaponSound( WeaponSound_t, float soundtime = 0.0f ) OVERRIDE;

    void DoMachineGunKick( float, float, float, float );

#ifdef CLIENT_DLL
    virtual CZMBaseCrosshair* GetWeaponCrosshair() const { return nullptr; }
#endif

    // How many bullets we fire per one "bullet", or clip "unit".
    virtual int GetBulletsPerShot() const { return 1; }

    virtual int GetMinBurst( void ) OVERRIDE { return 1; }
    virtual int GetMaxBurst( void ) OVERRIDE { return 1; }
    
#ifndef CLIENT_DLL
    // Volume = distance
    virtual float   GetAISoundVolume() const { return 1024.0f; }
    virtual void    PlayAISound() const;


    virtual bool IsTemplate() OVERRIDE;

    virtual void Materialize( void ) OVERRIDE;
#endif
    // Makes our weapons not cry about spawning.
    virtual void FallInit( void );
    // Override this so our guns don't disappear.
    virtual void SetPickupTouch( void ) OVERRIDE;
    virtual bool CanBeSelected( void ) OVERRIDE;
    // Never let anybody tell you're not beautiful even without any ammo, alright?
    // Let us always select this weapon even when we don't have any ammo for it.
    virtual bool AllowsAutoSwitchFrom( void ) const OVERRIDE { return false; }
    virtual void Drop( const Vector& ) OVERRIDE;

    // Add weapon slot flag.
    virtual void Equip( CBaseCombatCharacter* ) OVERRIDE;


    // Viewmodel/misc stuff
    float   CalcViewmodelBob( void ) OVERRIDE;
    void    AddViewmodelBob( CBaseViewModel*, Vector&, QAngle& ) OVERRIDE;
    Vector  GetBulletSpread( WeaponProficiency_t ) OVERRIDE;
    float   GetSpreadBias( WeaponProficiency_t ) OVERRIDE;

    const WeaponProficiencyInfo_t*          GetProficiencyValues() OVERRIDE;
    static const WeaponProficiencyInfo_t*   GetDefaultProficiencyValues();
    
    // Our stuff
    CZMPlayer*    GetPlayerOwner() const;
    virtual bool    CanBeDropped() const { return true; }
    virtual bool    CanPickupAmmo() const { return true; }
    virtual bool    IsInReload() const { return const_cast<CZMBaseWeapon*>( this )->CanReload() && m_bInReload; }
    virtual bool    CanAct() const; // Can we reload/attack?


    // ZMRTODO: Use config to load these.
    virtual float   GetAccuracyIncreaseRate() const { return 2.0f; }
    virtual float   GetAccuracyDecreaseRate() const { return 2.0f; }

    float           GetFirstInstanceOfAnimEventTime( int iSeq, int iAnimEvent ) const;


    inline int GetSlotFlag() const { return m_iSlotFlag; }
#ifndef CLIENT_DLL
    void                FreeWeaponSlot();


    virtual const char* GetDropAmmoName() const { return nullptr; }
    virtual int         GetDropAmmoAmount() const { return 1; }

    inline int          GetReserveAmmo() const { return m_nReserveAmmo; }
    inline void         SetReserveAmmo( int ammo ) { m_nReserveAmmo = ammo; }
#endif
    
protected:
#ifndef CLIENT_DLL
    void SaveReserveAmmo( CBaseCombatCharacter* );
    void TransferReserveAmmo( CBaseCombatCharacter* );
    // No support for secondary ammo since we'll never use it anyway, RIGHT?
    int m_nReserveAmmo;



    // Client side hit reg stuff
    virtual float GetMaxDamageDist( ZMUserCmdValidData_t& data ) const OVERRIDE;
    virtual int GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const OVERRIDE;


    inline int GetOverrideDamage() const { return m_nOverrideDamage; }
#endif

    inline void SetSlotFlag( int flags ) { m_iSlotFlag = flags; }
    int m_iSlotFlag;

private:
#ifndef CLIENT_DLL
    string_t        m_OverrideViewModel;
    string_t        m_OverrideWorldModel;

    int             m_nOverrideDamage;
#endif
    CNetworkVar( int, m_nOverrideClip1 );
};

inline CZMBaseWeapon* ToZMBaseWeapon( CBaseEntity* pEnt )
{
    if ( !pEnt || !pEnt->IsBaseCombatWeapon() )
        return nullptr;

    return static_cast<CZMBaseWeapon*>( pEnt );
}

inline CZMBaseWeapon* ToZMBaseWeapon( CBaseCombatWeapon* pEnt )
{
    return static_cast<CZMBaseWeapon*>( pEnt );
}
