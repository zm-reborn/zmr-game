#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"


#include "zmr_shareddefs.h"
#include "zmr_player_shared.h"

#include "zmr_sledge.h"


using namespace ZMWeaponConfig;



REGISTER_WEAPON_CONFIG( ZMCONFIGSLOT_SLEDGE, CZMSledgeConfig );


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponSledge, DT_ZM_WeaponSledge )


BEGIN_NETWORK_TABLE( CZMWeaponSledge, DT_ZM_WeaponSledge )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponSledge )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_sledge, CZMWeaponSledge );
PRECACHE_WEAPON_REGISTER( weapon_zm_sledge );


CZMWeaponSledge::CZMWeaponSledge()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
    SetConfigSlot( ZMCONFIGSLOT_SLEDGE );
}

const CZMSledgeConfig* CZMWeaponSledge::GetSledgeConfig() const
{
    return static_cast<const CZMSledgeConfig*>( GetWeaponConfig() );
}

bool CZMWeaponSledge::CanSecondaryAttack() const
{
    return true;
}

WeaponSound_t CZMWeaponSledge::GetSecondaryAttackSound() const
{
    return SPECIAL1;
}

bool CZMWeaponSledge::UsesAnimEvent( bool bSecondary ) const
{
    return true;
}


float CZMWeaponSledge::GetDamageForActivity( Activity act ) const
{
    // ZMRTODO: Stop using random values.
    float damage = BaseClass::GetDamageForActivity( act );


    auto* pConfig = GetSledgeConfig();

    if ( act == ACT_VM_HITCENTER2 )
    {
        damage *= random->RandomFloat( pConfig->flSecondaryRandomMultMin, pConfig->flSecondaryRandomMultMax );
    }
    else
    {
        damage *= random->RandomFloat( pConfig->flPrimaryRandomMultMin, pConfig->flPrimaryRandomMultMax );
    }

    return damage;
}

void CZMWeaponSledge::Hit( trace_t& traceHit, Activity iHitActivity )
{
    BaseClass::Hit( traceHit, iHitActivity );


    AddViewKick();

#ifndef CLIENT_DLL
    PlayAISound();
#endif
}

void CZMWeaponSledge::PrimaryAttack()
{
    if ( !CanPrimaryAttack() )
        return;


    Swing( false );


    // Setup our next attack times
    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

    auto* pMe = GetPlayerOwner();
    if ( pMe )
    {
        pMe->m_flNextAttack = m_flNextPrimaryAttack;
    }
}

void CZMWeaponSledge::SecondaryAttack()
{
    if ( !CanSecondaryAttack() )
        return;

        
    Swing( true );


    // Setup our next attack times
    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

    auto* pMe = GetPlayerOwner();
    if ( pMe )
    {
        pMe->m_flNextAttack = m_flNextPrimaryAttack;
    }
}
