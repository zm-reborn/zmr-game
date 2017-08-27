#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#endif

#include "zmr_basepump.h"


#ifdef CLIENT_DLL
#define CZMWeaponShotgun C_ZMWeaponShotgun
#endif

class CZMWeaponShotgun : public CZMBasePumpWeapon
{
public:
    DECLARE_CLASS( CZMWeaponShotgun, CZMBasePumpWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
#ifndef CLIENT_DLL
    DECLARE_ACTTABLE();
#endif

    CZMWeaponShotgun();


#ifndef CLIENT_DLL
    const char* GetDropAmmoName() OVERRIDE { return "item_box_buckshot"; };
    int GetDropAmmoAmount() OVERRIDE { return SIZE_AMMO_BUCKSHOT; };
#endif

    virtual const Vector& GetBulletSpread( void ) OVERRIDE
    {
        static Vector cone = VECTOR_CONE_10DEGREES;
        return cone;
    }
    
    virtual void AddViewKick( void ) OVERRIDE
    {
        CZMPlayer* pPlayer = ToZMPlayer( GetOwner() );

        if ( !pPlayer ) return;


        pPlayer->ViewPunch( QAngle( random->RandomFloat( -2, -1 ), random->RandomFloat( -2, 2 ), 0 ) );
    }
    
    virtual float GetFireRate( void ) OVERRIDE { return 0.55f; };

    virtual void PrimaryAttack( void ) OVERRIDE;


    virtual Activity GetReloadStartAct() OVERRIDE { return ACT_SHOTGUN_RELOAD_START; };
    virtual Activity GetReloadEndAct() OVERRIDE { return ACT_SHOTGUN_RELOAD_FINISH; };
    virtual Activity GetPumpAct() OVERRIDE { return ACT_SHOTGUN_PUMP; };
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponShotgun, DT_ZM_WeaponShotgun )

BEGIN_NETWORK_TABLE( CZMWeaponShotgun, DT_ZM_WeaponShotgun )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponShotgun )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_shotgun, CZMWeaponShotgun );
PRECACHE_WEAPON_REGISTER( weapon_zm_shotgun );

#ifndef CLIENT_DLL
acttable_t CZMWeaponShotgun::m_acttable[] = 
{
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
};
IMPLEMENT_ACTTABLE( CZMWeaponShotgun );
#endif

CZMWeaponShotgun::CZMWeaponShotgun()
{
    m_fMinRange1 = m_fMinRange2 = 0.0f;
    m_fMaxRange1 = m_fMaxRange2 = 500.0f;

    m_bFiresUnderwater = false;

#ifndef CLIENT_DLL
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
#endif
}

void CZMWeaponShotgun::PrimaryAttack( void )
{
    CZMPlayer* pPlayer = GetPlayerOwner();

    if ( !pPlayer ) return;


    m_bNeedPump = true;


    // MUST call sound before removing a round from the clip of a CMachineGun
    WeaponSound( SINGLE );

    pPlayer->DoMuzzleFlash();

    SendWeaponAnim( ACT_VM_PRIMARYATTACK );

    // Don't fire again until fire animation has completed
    m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
    m_iClip1 -= 1;

    // player "shoot" animation
    pPlayer->SetAnimation( PLAYER_ATTACK1 );


    FireBulletsInfo_t info(
        7,
        pPlayer->Weapon_ShootPosition(),
        ((CBasePlayer*)pPlayer)->GetAutoaimVector( AUTOAIM_10DEGREES ),
        GetBulletSpread(),
        MAX_TRACE_LENGTH,
        m_iPrimaryAmmoType );

    info.m_pAttacker = pPlayer;
    
    // Fire the bullets, and force the first shot to be perfectly accuracy
    FireBullets( info );
    
    QAngle punch;
    punch.Init( SharedRandomFloat( "shotgunpax", -2, -1 ), SharedRandomFloat( "shotgunpay", -2, 2 ), 0 );
    pPlayer->ViewPunch( punch );

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
    }


    m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}
