#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif


#include "zmr_shareddefs.h"
#include "zmr_mac10.h"


#include "zmr_player_shared.h"


using namespace ZMWeaponConfig;

//
CZMMac10Config::CZMMac10Config( const char* wepname, const char* configpath ) : CZMBaseWeaponConfig( wepname, configpath )
{
    flEasyDampen = 0.0f;
    flVerticalKick = 0.0f;
    flSlideLimit = 0.0f;
}

void CZMMac10Config::LoadFromConfig( KeyValues* kv )
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

bool CZMMac10Config::OverrideFromConfig( KeyValues* kv )
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

KeyValues* CZMMac10Config::ToKeyValues() const
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

REGISTER_WEAPON_CONFIG( ZMCONFIGSLOT_MAC10, CZMMac10Config );
//


//
IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponMac10, DT_ZM_WeaponMac10 )

BEGIN_NETWORK_TABLE( CZMWeaponMac10, DT_ZM_WeaponMac10 )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponMac10 )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_mac10, CZMWeaponMac10 );
PRECACHE_WEAPON_REGISTER( weapon_zm_mac10 );


//
CZMWeaponMac10::CZMWeaponMac10()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_MAC10 );
}

const CZMMac10Config* CZMWeaponMac10::GetMac10Config() const
{
    return static_cast<const CZMMac10Config*>( GetWeaponConfig() );
}

void CZMWeaponMac10::AddViewKick()
{
    auto* pConfig = GetMac10Config();

	DoMachineGunKick(
        pConfig->flEasyDampen,
        pConfig->flVerticalKick,
        m_fFireDuration,
        pConfig->flSlideLimit );
}
