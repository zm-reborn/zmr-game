#include "cbase.h"

#include "npcevent.h"
#include "eventlist.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponFists C_ZMWeaponFists
#endif

class CZMWeaponFists : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponFists, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

    CZMWeaponFists();

#ifdef CLIENT_DLL
    bool ShouldDrawPickup() OVERRIDE { return false; };
    bool ShouldDraw() OVERRIDE { return false; };
#endif

    float GetRange() OVERRIDE { return 45.0f; };
    float GetFireRate() OVERRIDE { return 0.65f; };

    void AddViewKick() OVERRIDE
    {
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( !pPlayer ) return;
	    pPlayer->ViewPunch( QAngle( random->RandomFloat( 0.5f, 1.0f ), random->RandomFloat( -1.0f, -0.5f ), 0.0f ) );
    };


#ifndef CLIENT_DLL
    bool CanBeDropped() OVERRIDE { return false; };


    void Operator_HandleAnimEvent( animevent_t*, CBaseCombatCharacter* ) OVERRIDE;
#else
    bool OnFireEvent( C_BaseViewModel*, const Vector&, const QAngle&, int event, const char* ) OVERRIDE;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponFists, DT_ZM_WeaponFists )

BEGIN_NETWORK_TABLE( CZMWeaponFists, DT_ZM_WeaponFists )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponFists )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_fists, CZMWeaponFists );
PRECACHE_WEAPON_REGISTER( weapon_zm_fists );

#ifndef CLIENT_DLL
acttable_t	CZMWeaponFists::m_acttable[] = 
{
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,                  true },
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponFists );
#endif

CZMWeaponFists::CZMWeaponFists()
{
#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_NONE );
#endif
}

#ifndef CLIENT_DLL
void CZMWeaponFists::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case AE_ZM_MELEEHIT:
		HandleAnimEventMeleeHit( pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#else
bool CZMWeaponFists::OnFireEvent( C_BaseViewModel* pViewModel, const Vector& origin, const QAngle& angles, int event, const char* options )
{
    if ( event == AE_ZM_MELEEHIT )
    {
        HandleAnimEventMeleeHit( GetOwner() );
        return true;
    }

    return false;
}
#endif