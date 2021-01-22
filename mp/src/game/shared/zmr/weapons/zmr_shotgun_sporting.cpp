#include "cbase.h"

#ifndef CLIENT_DLL
#include "items.h"
#else
#include "prediction.h"
#endif

#include "zmr_basepump.h"


#ifdef CLIENT_DLL
#define CZMWeaponShotgunSporting C_ZMWeaponShotgunSporting
#endif

class CZMWeaponShotgunSporting : public CZMBaseWeapon
{
public:
    DECLARE_CLASS( CZMWeaponShotgunSporting, CZMBaseWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CZMWeaponShotgunSporting();


    bool IsFiringBothBarrels() const { return IsInSecondaryAttack(); }


    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;
    virtual bool Reload() OVERRIDE;
    virtual void SecondaryAttackEffects( WeaponSound_t wpnsound ) OVERRIDE;


    void ShootBarrels( bool bWantBoth );


#ifdef GAME_DLL
    virtual int GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const OVERRIDE;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponShotgunSporting, DT_ZM_WeaponShotgunSporting )

BEGIN_NETWORK_TABLE( CZMWeaponShotgunSporting, DT_ZM_WeaponShotgunSporting )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponShotgunSporting )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_shotgun_sporting, CZMWeaponShotgunSporting );

PRECACHE_WEAPON_REGISTER( weapon_zm_shotgun_sporting );


CZMWeaponShotgunSporting::CZMWeaponShotgunSporting()
{
    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_SHOTGUNSPORTING );
}

void CZMWeaponShotgunSporting::PrimaryAttack()
{
    ShootBarrels( false );
}

void CZMWeaponShotgunSporting::SecondaryAttack()
{
    ShootBarrels( true );
}

void CZMWeaponShotgunSporting::ShootBarrels( bool bWantBoth )
{
    if ( !CanAct( bWantBoth ? WEPACTION_ATTACK2 : WEPACTION_ATTACK ) ) return;


    if ( m_flNextPrimaryAttack > gpGlobals->curtime )
        return;


    // If my clip is empty (and I use clips) start reload
    if ( UsesClipsForAmmo1() && !m_iClip1 ) 
    {
        Reload();
        return;
    }


    bool bShootSingle = m_iClip1 < 2 || !bWantBoth;


    float delay = GetFireRate();

    m_flNextSecondaryAttack = gpGlobals->curtime + delay;
    m_flNextPrimaryAttack = m_flNextSecondaryAttack;
    

    if ( bShootSingle )
    {
        Shoot( -1, -1, -1, GetWeaponConfig()->primary.flRange );
    }
    else
    {
        Shoot( m_iPrimaryAmmoType, -1, 2, GetWeaponConfig()->secondary.flRange, true );
    }
}

bool CZMWeaponShotgunSporting::Reload()
{
    bool bReloadSingle = m_iClip1 >= 1;

    return DefaultReload( GetMaxClip1(), GetMaxClip2(), bReloadSingle ? ACT_VM_RELOAD_START : ACT_VM_RELOAD );
}

void CZMWeaponShotgunSporting::SecondaryAttackEffects( WeaponSound_t wpnsound )
{
    // Play double shot for secondary.
    BaseClass::SecondaryAttackEffects( WeaponSound_t::WPN_DOUBLE );
}

#ifdef GAME_DLL
int CZMWeaponShotgunSporting::GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const
{
    int bullets = GetBulletsPerShot();
    return IsInSecondaryAttack() ? (bullets * 2) : bullets;
}
#endif
