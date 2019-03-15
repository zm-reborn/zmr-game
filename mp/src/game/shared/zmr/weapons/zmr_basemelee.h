#pragma once

#include "zmr_base.h"


#ifdef CLIENT_DLL
#define CZMBaseMeleeWeapon C_ZMBaseMeleeWeapon
#endif

class CZMBaseMeleeWeapon : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMBaseMeleeWeapon, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
    DECLARE_DATADESC();


    CZMBaseMeleeWeapon();


#ifdef CLIENT_DLL
    virtual CZMBaseCrosshair* GetWeaponCrosshair() const OVERRIDE { return ZMGetCrosshair( "Melee" ); }
#endif

    virtual void ItemPostFrame() OVERRIDE;

    virtual bool Deploy() OVERRIDE;


    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;


    virtual bool UsesAnimEvent( bool bSecondary ) const { return false; }
    
    virtual float GetRange() const { return 10.0f; }
    virtual	float GetDamageForActivity( Activity hitActivity ) const { return 1.0f; }

    virtual bool CanPrimaryAttack() const { return true; }
    virtual bool CanSecondaryAttack() const { return false; }
    virtual Activity GetPrimaryAttackActivity() OVERRIDE { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity() OVERRIDE { return ACT_VM_HITCENTER2; }

    virtual WeaponSound_t GetPrimaryAttackSound() const { return SINGLE; }
    virtual WeaponSound_t GetSecondaryAttackSound() const { return SINGLE; }

#ifndef CLIENT_DLL
    // Lower the sound distance a bit.
    virtual float GetAISoundVolume() const OVERRIDE { return 500.0f; }
#endif

#ifdef GAME_DLL
    virtual float GetMaxDamageDist( ZMUserCmdValidData_t& data ) const OVERRIDE;
#endif

protected:
    virtual void Swing( bool bSecondary );
    virtual void StartHit( trace_t* traceRes = nullptr, Activity iActivityDamage = ACT_VM_HITCENTER );
    virtual void Hit( trace_t& tr, Activity act );

    void ChooseIntersectionPoint( trace_t& tr, const Vector& mins, const Vector& maxs );

    virtual void TraceMeleeAttack( trace_t& tr );

    void ImpactEffect( trace_t& tr );
    bool ImpactWater( const Vector& vecStart, const Vector& vecEnd );



#ifndef CLIENT_DLL
    virtual void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) OVERRIDE;
#else
    virtual bool OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options ) OVERRIDE;
#endif


private:
    CNetworkVar( float, m_flAttackHitTime );
};
