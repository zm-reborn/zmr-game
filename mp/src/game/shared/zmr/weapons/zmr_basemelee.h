//#include "basecombatcharacter.h"

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


    void ItemPostFrame() OVERRIDE;

    virtual float GetRange() const { return 10.0f; }
    virtual	float GetDamageForActivity( Activity hitActivity ) const { return 1.0f; }
    virtual Activity GetPrimaryAttackActivity( void ) OVERRIDE { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity( void ) OVERRIDE { return ACT_VM_HITCENTER2; }

#ifndef CLIENT_DLL
    // Lower the sound distance a bit.
    virtual float GetAISoundVolume() const OVERRIDE { return 500.0f; }
#endif

protected:
    virtual void Swing( bool bSecondary, const bool bUseAnimationEvent = true );
    virtual void HandleAnimEventMeleeHit();
    virtual void Hit( trace_t&, Activity );

    void ChooseIntersectionPoint( trace_t& tr, const Vector& mins, const Vector& maxs );

    virtual void TraceMeleeAttack( trace_t& tr );

    void ImpactEffect( trace_t& tr );
    bool ImpactWater( const Vector& vecStart, const Vector& vecEnd );
};
