#include "cbase.h"

#include "zmr_mac10.h"

#ifdef CLIENT_DLL
#define CZMWeaponAR9mm C_ZMWeaponAR9mm
#endif


#define DELAY_AFTER_BURST       0.5f


#define NOT_IN_BURST            0

#define BURST_SHOTS             3

class CZMWeaponAR9mm : public CZMWeaponMac10
{
public:
	DECLARE_CLASS( CZMWeaponAR9mm, CZMWeaponMac10 );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

    CZMWeaponAR9mm();

    virtual bool Deploy() OVERRIDE;

    virtual void ItemPostFrame() OVERRIDE;

    virtual void SecondaryAttack() OVERRIDE;


    bool IsFiringBurst() const;

private:
    void FireBurstBullet( int index = 0 );

    CNetworkVar( int, m_nBurstShots );
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponAR9mm, DT_ZM_WeaponAR9mm )

BEGIN_NETWORK_TABLE( CZMWeaponAR9mm, DT_ZM_WeaponAR9mm )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_nBurstShots ) ),
#else
    SendPropInt( SENDINFO( m_nBurstShots ), Q_log2( BURST_SHOTS ) + 1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponAR9mm )
    DEFINE_PRED_FIELD( m_nBurstShots, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_ar9mm, CZMWeaponAR9mm );
PRECACHE_WEAPON_REGISTER( weapon_zm_ar9mm );

CZMWeaponAR9mm::CZMWeaponAR9mm()
{
    m_nBurstShots = NOT_IN_BURST;

    SetSlotFlag( ZMWEAPONSLOT_LARGE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_AR9MM );
}

bool CZMWeaponAR9mm::Deploy()
{
    m_nBurstShots = NOT_IN_BURST;
    return BaseClass::Deploy();
}

void CZMWeaponAR9mm::ItemPostFrame()
{
    BaseClass::ItemPostFrame();

    if ( IsFiringBurst() && m_flNextSecondaryAttack <= gpGlobals->curtime )
    {
        FireBurstBullet( m_nBurstShots );
        
        
        if ( m_nBurstShots >= BURST_SHOTS )
        {
            m_nBurstShots = NOT_IN_BURST;

            m_flNextPrimaryAttack = gpGlobals->curtime + DELAY_AFTER_BURST;
            m_flNextSecondaryAttack = m_flNextPrimaryAttack;
        }
    }
}

void CZMWeaponAR9mm::SecondaryAttack()
{
    if ( !CanAct( WEPACTION_ATTACK2 ) ) return;

    if ( m_flNextPrimaryAttack > gpGlobals->curtime )
        return;

    // Already burst firing.
    if ( IsFiringBurst() )
    {
        return;
    }

    FireBurstBullet( 0 );
}

bool CZMWeaponAR9mm::IsFiringBurst() const
{
    return m_nBurstShots != NOT_IN_BURST;
}

void CZMWeaponAR9mm::FireBurstBullet( int index )
{
    // If my clip is empty (and I use clips) start reload
    if ( UsesClipsForAmmo1() && !m_iClip1 ) 
    {
        m_nBurstShots = NOT_IN_BURST;

        Reload();
        return;
    }

    float delay = GetWeaponConfig()->secondary.flFireRate;

    m_flNextSecondaryAttack = gpGlobals->curtime + delay;
    m_flNextPrimaryAttack = m_flNextSecondaryAttack + GetWeaponConfig()->primary.flFireRate;
    

    Shoot( m_iPrimaryAmmoType, -1, -1, GetWeaponConfig()->secondary.flRange, true );

    m_nBurstShots = index + 1;
}
