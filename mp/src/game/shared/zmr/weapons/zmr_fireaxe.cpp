#include "cbase.h"

#include "zmr_sledge.h"



using namespace ZMWeaponConfig;

#ifdef CLIENT_DLL
#define CZMWeaponFireAxe C_ZMWeaponFireAxe
#endif

REGISTER_WEAPON_CONFIG( ZMCONFIGSLOT_FIREAXE, CZMSledgeConfig );

class CZMWeaponFireAxe : public CZMWeaponSledge
{
public:
	DECLARE_CLASS( CZMWeaponFireAxe, CZMWeaponSledge );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CZMWeaponFireAxe();
};
IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponFireAxe, DT_ZM_WeaponFireAxe )


BEGIN_NETWORK_TABLE( CZMWeaponFireAxe, DT_ZM_WeaponFireAxe )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponFireAxe )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_fireaxe, CZMWeaponFireAxe );
PRECACHE_WEAPON_REGISTER( weapon_zm_fireaxe );


CZMWeaponFireAxe::CZMWeaponFireAxe()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
    SetConfigSlot( ZMCONFIGSLOT_FIREAXE );
}
