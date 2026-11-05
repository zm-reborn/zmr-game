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


    virtual void ItemBusyFrame() override;
    virtual void ItemPostFrame() override;

    virtual bool Deploy() override;


    virtual void PrimaryAttack() override;
    virtual void SecondaryAttack() override;


    virtual bool UsesAnimEvent( bool bSecondary ) const { return false; }
    
    virtual float GetRange() const;
    virtual	float GetDamageForActivity( Activity hitActivity ) const;

    virtual bool CanPrimaryAttack() const { return true; }
    virtual bool CanSecondaryAttack() const { return false; }
    virtual Activity GetPrimaryAttackActivity() override { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity() override { return ACT_VM_HITCENTER2; }

    virtual WeaponSound_t GetPrimaryAttackSound() const { return SINGLE; }
    virtual WeaponSound_t GetSecondaryAttackSound() const { return SINGLE; }

#ifndef CLIENT_DLL
    // Lower the sound distance a bit.
    virtual float GetAISoundVolume() const override { return 500.0f; }
#endif

#ifdef GAME_DLL
    virtual float GetMaxDamageDist( ZMUserCmdValidData_t& data ) const override;
#endif

protected:
    virtual void Swing( bool bSecondary );
    virtual void StartHit( trace_t* traceRes = nullptr, Activity iActivityDamage = ACT_VM_HITCENTER, bool bJustTrace = false );
    virtual void Hit( trace_t& tr, Activity act );

    void ChooseIntersectionPoint( trace_t& tr, const Vector& mins, const Vector& maxs );

    virtual void TraceMeleeAttack( trace_t& tr );

    void ImpactEffect( trace_t& tr );
    bool ImpactWater( const Vector& vecStart, const Vector& vecEnd );



#ifndef CLIENT_DLL
    virtual void Operator_HandleAnimEvent( animevent_t* pEvent, CBaseCombatCharacter* pOperator ) override;
#else
    virtual bool OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options ) override;
#endif


private:
    CNetworkVar( float, m_flAttackHitTime );
};
