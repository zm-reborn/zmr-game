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

    virtual float GetRange() { return 10.0f; };
    virtual	float GetDamageForActivity( Activity hitActivity ) { return	1.0f; };
    virtual Activity GetPrimaryAttackActivity( void ) OVERRIDE { return ACT_VM_HITCENTER; };
	virtual Activity GetSecondaryAttackActivity( void ) OVERRIDE { return ACT_VM_HITCENTER2; };

protected:
    //void Swing( bool );
    //Activity ChooseIntersectionPointAndActivity( trace_t&, const Vector&, const Vector&, CBasePlayer* );

    virtual void Hit( trace_t&, Activity );
    virtual void HandleAnimEventMeleeHit( CBaseCombatCharacter* );

    void ImpactEffect( trace_t& );
    bool ImpactWater( const Vector&, const Vector& );
};