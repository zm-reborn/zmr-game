#pragma once

//#include "hl2mp/hl2mp_player_shared.h"
#include "basecombatweapon_shared.h"
//#include "hl2mp/weapon_hl2mpbase.h"
//#include "hl2mp/hl2mp_weapon_parse.h"

#ifdef CLIENT_DLL
#include "c_zmr_crosshair.h"
#endif
#include "zmr_weapon_parse.h"
#include "zmr_usercmdvalid.h"
#include "zmr_weaponconfig.h"

#include "zmr_player_shared.h"



#ifdef CLIENT_DLL
#define CZMBaseWeapon C_ZMBaseWeapon
#endif

/*
    NOTE:

    You must add custom animation events in eventlist.h && eventlist.cpp
*/

// 1 is constrained.
#define SF_ZMWEAPON_TEMPLATE        ( 1 << 1 )


enum ZMWepActionType_t
{
    WEPACTION_GENERIC = 0,

    WEPACTION_RELOAD,

    WEPACTION_ATTACK,
    WEPACTION_ATTACK2,
    WEPACTION_ATTACK3,
};


class CZMBaseWeapon : public CBaseCombatWeapon, public CZMUserCmdHitWepValidator
{
public:
	DECLARE_CLASS( CZMBaseWeapon, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS();
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#else
    DECLARE_DATADESC();
#endif

	CZMBaseWeapon();
	~CZMBaseWeapon();

    static bool IsDebugging();
    
#ifdef CLIENT_DLL
    virtual void Spawn() override;
    virtual void PostDataUpdate( DataUpdateType_t updateType ) override;
#endif
    virtual void Precache() override;

    virtual void ItemPostFrame() override;

    virtual bool Deploy() override;
    virtual bool DefaultDeploy();
    virtual bool Holster( CBaseCombatWeapon* pSwitchingTo = nullptr ) override;

    virtual acttable_t* ActivityList( int& iActivityCount ) override;
    virtual Activity GetPrimaryAttackActivity() override;
    //virtual Activity GetSecondaryAttackActivity() override;
    virtual Activity GetDrawActivity() override;
    virtual Activity GetIdleActivity() const;

    virtual void WeaponIdle() override;

    bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
    virtual	void CheckReload() override;
    virtual bool Reload() override;
    // NOTE: Always use this to get the damage from .txt file.
    virtual void FireBullets( const FireBulletsInfo_t& info ) override;
    virtual void FireBullets( int numShots, int iAmmoType, float flMaxDist );
    virtual void PrimaryAttack() override;
    virtual void Shoot( int iAmmoType = -1, int nBullets = -1, int nAmmo = -1, float flMaxRange = -1.0f, bool bSecondary = false );
    virtual void PrimaryAttackEffects( WeaponSound_t wpnsound );
    virtual void SecondaryAttackEffects( WeaponSound_t wpnsound );
    virtual void SecondaryAttack() override;

    virtual bool DryFire( bool bForce );


    virtual void        SetViewModel() override;

#ifdef CLIENT_DLL
    virtual void    ClientThink() override;
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) const override;
    void            UpdateGlow();

    virtual bool    GlowOccluded() const override { return false; }
    virtual bool    GlowUnoccluded() const override { return true; }


    virtual ShadowType_t ShadowCastType() override;

    virtual void OnDataChanged( DataUpdateType_t ) override;
    virtual bool ShouldPredict() override;
#endif
    virtual bool IsPredicted() const override { return true; };

    virtual void SetWeaponVisible( bool visible ) override;

    void WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f ) override;

    void DoMachineGunKick( float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

#ifdef CLIENT_DLL
    virtual CZMBaseCrosshair* GetWeaponCrosshair() const;

    virtual float GetScopeFOVModifier() const;
#endif

    
    virtual int GetMinBurst() override { return 1; }
    virtual int GetMaxBurst() override { return 1; }
    
#ifndef CLIENT_DLL
    // Volume = distance
    virtual float   GetAISoundVolume() const { return 1024.0f; }
    virtual void    PlayAISound() const;


    virtual bool IsTemplate() override;

    virtual void Materialize() override;

    virtual void FallThink() override;
#endif
    // Makes our weapons not cry about spawning.
    virtual void FallInit();
    // Override this so our guns don't disappear.
    virtual void SetPickupTouch() override;
    virtual void OnPickedUp( CBaseCombatCharacter* pNewOwner ) override;
    virtual bool CanBeSelected() override;
    // Never let anybody tell you're not beautiful even without any ammo, alright?
    // Let us always select this weapon even when we don't have any ammo for it.
    virtual bool AllowsAutoSwitchFrom() const override { return false; }
    virtual void Drop( const Vector& vecVelocity ) override;

    // Add weapon slot flag.
    virtual void Equip( CBaseCombatCharacter* pCharacter ) override;


    // Viewmodel/misc stuff
    float   CalcViewmodelBob() override;
    void    AddViewmodelBob( CBaseViewModel* pVM, Vector& origin, QAngle& angles ) override;
    
    // Our stuff
    CZMPlayer*    GetPlayerOwner() const;
    virtual bool    CanBeDropped() const { return true; }
    virtual bool    CanPickupAmmo() const { return true; }
    virtual bool    IsInReload() const { return const_cast<CZMBaseWeapon*>( this )->CanReload() && m_bInReload2; }
    virtual bool    CanAct( ZMWepActionType_t type = WEPACTION_GENERIC ) const; // Can we reload/attack?

    virtual bool    IsZoomed() const { return false; }

    virtual void IncrementClip();
    virtual bool ShouldIncrementClip() const;
    virtual void CancelReload();
    virtual bool ShouldCancelReload() const;
    virtual void StopReload();

    // We have animations for when the gun is empty?
    virtual bool UsesDryActivity( Activity act );


    virtual void AddViewKick() override;

    virtual bool IsInSecondaryAttack() const;
    const ZMWeaponConfig::CZMBaseWeaponConfig* GetWeaponConfig() const;
	virtual CHudTexture const* GetSpriteActive() const override;
	virtual CHudTexture const* GetSpriteInactive() const override;
    virtual CHudTexture const* GetSpriteAmmo() const override;
	virtual const char*     GetViewModel( int vmIndex = 0 ) const override;
	virtual const char*     GetWorldModel() const override;
    virtual int             GetMaxClip1() const override;
    virtual int             GetMaxClip2() const override;
    virtual int             GetDefaultClip1() const override;
    virtual const char*     GetAnimPrefix() const;
    virtual int             GetSlot() const override;
    virtual int             GetPosition() const override;
    virtual int             GetWeight() const override;
    virtual char const*     GetName() const override;
    virtual char const*     GetPrintName() const override;
    virtual char const*     GetShootSound( int iIndex ) const override;
    float                   GetPrimaryFireRate() const;
    float                   GetAccuracyIncreaseRate() const;
    float                   GetAccuracyDecreaseRate() const;
    float                   GetPenetrationDmgMult() const;
    int                     GetMaxPenetrations() const;
    float                   GetMaxPenetrationDist() const;
    virtual Vector          GetBulletSpread() const;
    virtual float           GetFireRate() override;
    virtual float           GetReloadTime() const;
    // How many bullets we fire per one "bullet", or clip "unit".
    virtual int             GetBulletsPerShot() const;


    float           GetFirstInstanceOfAnimEventTime( int iSeq, int iAnimEvent, bool bReturnOption = false ) const;
    Activity        GetActivityOrFallback( Activity act, Activity fallback ) const;

    inline int GetSlotFlag() const { return m_iSlotFlag; }
#ifndef CLIENT_DLL
    void                FreeWeaponSlot();


    virtual const char* GetDropAmmoName() const;
    virtual int         GetDropAmmoAmount() const;

    inline int          GetReserveAmmo() const { return m_nReserveAmmo; }
    inline void         SetReserveAmmo( int ammo ) { m_nReserveAmmo = ammo; }
#endif

    inline bool IsHoldingAfterFiring() const { return m_bHoldingAfterFiring; }
    
protected:
#ifndef CLIENT_DLL
    void SaveReserveAmmo( CBaseCombatCharacter* pOwner );
    void TransferReserveAmmo( CBaseCombatCharacter* pOwner );
    // No support for secondary ammo since we'll never use it anyway, RIGHT?
    int m_nReserveAmmo;



    // Client side hit reg stuff
    virtual float GetMaxDamageDist( ZMUserCmdValidData_t& data ) const override;
    virtual int GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const override;
    virtual int GetMaxNumPenetrate( ZMUserCmdValidData_t& data ) const override;


    inline int GetOverrideDamage() const { return m_nOverrideDamage; }
#endif

    inline void SetSlotFlag( int flags ) { m_iSlotFlag = flags; }
    int m_iSlotFlag;

    inline ZMWeaponConfig::WeaponConfigSlot_t GetConfigSlot() const { return m_iConfigSlot; }
    inline void SetConfigSlot( ZMWeaponConfig::WeaponConfigSlot_t slot ) { m_iConfigSlot = slot; }
    void AssignWeaponConfigSlot();


    // Override baseclass m_bInReload
    // Mainly to fix prediction errors, since m_bInReload is not networked.
    CNetworkVar( bool, m_bInReload2 );

    CNetworkVar( float, m_flNextClipFillTime );
    CNetworkVar( bool, m_bHoldingAfterFiring );

#ifdef GAME_DLL
    void ReleaseConstraint();
    IPhysicsConstraint* GetConstraint() const { return m_pConstraint; }
#endif

private:
#ifndef CLIENT_DLL
    string_t        m_OverrideViewModel;
    string_t        m_OverrideWorldModel;

    int             m_nOverrideDamage;

    IPhysicsConstraint* m_pConstraint;
#endif
    CNetworkVar( int, m_nOverrideClip1 );


    CNetworkVar( bool, m_bCanCancelReload );

#define MAX_CUSTOM_SCRIPTFILENAME_LENGTH        128
#ifdef GAME_DLL
    CNetworkVar( string_t, m_sScriptFileName );
#else
    char m_sScriptFileName[MAX_CUSTOM_SCRIPTFILENAME_LENGTH];
#endif


    ZMWeaponConfig::WeaponConfigSlot_t m_iConfigSlot;
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
