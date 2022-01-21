#include "cbase.h"

#include "zmr_revolver.h"


#ifdef CLIENT_DLL
#define CZMWeaponSnubnose C_ZMWeaponSnubnose
#endif

class CZMWeaponSnubnose : public CZMWeaponRevolver
{
public:
	DECLARE_CLASS( CZMWeaponSnubnose, CZMWeaponRevolver );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();


    CZMWeaponSnubnose();
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponSnubnose, DT_ZM_WeaponSnubnose )

BEGIN_NETWORK_TABLE( CZMWeaponSnubnose, DT_ZM_WeaponSnubnose )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponSnubnose )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_snubnose, CZMWeaponSnubnose );
PRECACHE_WEAPON_REGISTER( weapon_zm_snubnose );

CZMWeaponSnubnose::CZMWeaponSnubnose()
{
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_SNUBNOSE );
}
