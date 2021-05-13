#include "cbase.h"
#include "in_buttons.h"

#include "zmr_shareddefs.h"
#include "zmr_basethrowable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
#define CZMWeaponPipebomb C_ZMWeaponPipebomb
#endif

class CZMWeaponPipebomb : public CZMBaseThrowableWeapon
{
public:
    DECLARE_CLASS( CZMWeaponPipebomb, CZMBaseThrowableWeapon );
    DECLARE_NETWORKCLASS(); 
    DECLARE_PREDICTABLE();

    CZMWeaponPipebomb();
    ~CZMWeaponPipebomb();


    virtual void PrimaryAttack() OVERRIDE;

    virtual const char* GetProjectileClassname() const OVERRIDE;
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponPipebomb, DT_ZM_WeaponPipebomb )

BEGIN_NETWORK_TABLE( CZMWeaponPipebomb, DT_ZM_WeaponPipebomb )

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponPipebomb )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_pipebomb, CZMWeaponPipebomb );
PRECACHE_WEAPON_REGISTER( weapon_zm_pipebomb );

REGISTER_WEAPON_CONFIG( ZMWeaponConfig::ZMCONFIGSLOT_PIPEBOMB, CZMBaseThrowableConfig );

CZMWeaponPipebomb::CZMWeaponPipebomb()
{
    SetSlotFlag( ZMWEAPONSLOT_EQUIPMENT );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_PIPEBOMB );
}

CZMWeaponPipebomb::~CZMWeaponPipebomb()
{
}

void CZMWeaponPipebomb::PrimaryAttack()
{
    SendWeaponAnim( ACT_VM_PRIMARYATTACK_1 );
    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

    // Jump the throw state to drawback, so base code can update the state.
    SetThrowState( THROWSTATE_DRAW_BACK );
}

const char* CZMWeaponPipebomb::GetProjectileClassname() const
{
    return "grenade_pipebomb";
}
