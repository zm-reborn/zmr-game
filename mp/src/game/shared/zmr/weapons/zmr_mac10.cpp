#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif


#include "zmr_shareddefs.h"
#include "zmr_weaponconfig.h"
#include "zmr_base.h"


#include "zmr_player_shared.h"


#ifdef CLIENT_DLL
#define CZMWeaponMac10 C_ZMWeaponMac10
#endif


using namespace ZMWeaponConfig;

class CZMMac10Config : public CZMBaseWeaponConfig
{
public:
    CZMMac10Config( const char* wepname, const char* configpath ) : CZMBaseWeaponConfig( wepname, configpath )
    {
        flEasyDampen = 0.0f;
        flVerticalKick = 0.0f;
        flSlideLimit = 0.0f;
    }

    virtual void LoadFromConfig( KeyValues* kv ) OVERRIDE
    {
        CZMBaseWeaponConfig::LoadFromConfig( kv );

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            flEasyDampen = inner->GetFloat( "easy_dampen", 0.5f );
            flVerticalKick = inner->GetFloat( "max_vertical_kick", 2.0f );
            flSlideLimit = inner->GetFloat( "slide_limit", 1.0f );
        }
    }

    virtual bool OverrideFromConfig( KeyValues* kv ) OVERRIDE
    {
        auto ret = CZMBaseWeaponConfig::OverrideFromConfig( kv );

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            OVERRIDE_FROM_WEPCONFIG_F( inner, easy_dampen, flEasyDampen );
            OVERRIDE_FROM_WEPCONFIG_F( inner, max_vertical_kick, flVerticalKick );
            OVERRIDE_FROM_WEPCONFIG_F( inner, slide_limit, flSlideLimit );
        }

        return ret;
    }

    virtual KeyValues* ToKeyValues() const OVERRIDE
    {
        auto* kv = CZMBaseWeaponConfig::ToKeyValues();

        KeyValues* inner;

        inner = kv->FindKey( "PrimaryAttack" );
        if ( inner )
        {
            inner->SetFloat( "easy_dampen", flEasyDampen );
            inner->SetFloat( "max_vertical_kick", flVerticalKick );
            inner->SetFloat( "slide_limit", flSlideLimit );
        }

        return kv;
    }

    
    float flEasyDampen;
    float flVerticalKick;
    float flSlideLimit;
};

REGISTER_WEAPON_CONFIG( ZMCONFIGSLOT_MAC10, CZMMac10Config );


class CZMWeaponMac10 : public CZMBaseWeapon
{
public:
	DECLARE_CLASS( CZMWeaponMac10, CZMBaseWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponMac10();


    const CZMMac10Config* GetMac10Config() const { return static_cast<const CZMMac10Config*>( GetWeaponConfig() ); }

    
    virtual void AddViewKick() OVERRIDE
    {
        auto* pConfig = GetMac10Config();

	    DoMachineGunKick(
            pConfig->flEasyDampen,
            pConfig->flVerticalKick,
            m_fFireDuration,
            pConfig->flSlideLimit );
    }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponMac10, DT_ZM_WeaponMac10 )

BEGIN_NETWORK_TABLE( CZMWeaponMac10, DT_ZM_WeaponMac10 )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponMac10 )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_mac10, CZMWeaponMac10 );
PRECACHE_WEAPON_REGISTER( weapon_zm_mac10 );

CZMWeaponMac10::CZMWeaponMac10()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_MAC10 );
}
