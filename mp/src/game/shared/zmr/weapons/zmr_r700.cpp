#include "cbase.h"
#include "in_buttons.h"

#include "zmr_rifle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


enum ZMScopeZoomLevel_t
{
    SCOPEZOOM_NONE = -1,

    SCOPEZOOM_1X,
    SCOPEZOOM_2X,
    SCOPEZOOM_4X,

    SCOPEZOOM_MAX
};


#ifdef CLIENT_DLL
#define CZMWeaponR700 C_ZMWeaponR700
#endif

class CZMWeaponR700 : public CZMWeaponRifle
{
public:
    DECLARE_CLASS( CZMWeaponR700, CZMWeaponRifle );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CZMWeaponR700();


    virtual void Precache() OVERRIDE;

    virtual Vector GetBulletSpread() const OVERRIDE;
#ifdef CLIENT_DLL
    virtual float GetScopeFOVModifier() const OVERRIDE;
#endif

    virtual void ItemPostFrame() OVERRIDE;

    virtual void Equip( CBaseCombatCharacter* pCharacter ) OVERRIDE;


    void ChangeZoom();

private:
    CNetworkVar( ZMScopeZoomLevel_t, m_iZoomLevel );
};


IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponR700, DT_ZM_WeaponR700 )

BEGIN_NETWORK_TABLE( CZMWeaponR700, DT_ZM_WeaponR700 )
#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO( m_iZoomLevel ) ),
#else
    SendPropInt( SENDINFO( m_iZoomLevel ), Q_log2( SCOPEZOOM_MAX ) + 1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponR700 )
    DEFINE_PRED_FIELD( m_iZoomLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_r700, CZMWeaponR700 );
PRECACHE_WEAPON_REGISTER( weapon_zm_r700 );


CZMWeaponR700::CZMWeaponR700()
{
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_R700 );

    m_iZoomLevel = SCOPEZOOM_2X;
}

void CZMWeaponR700::Precache()
{
    BaseClass::Precache();
    
    PrecacheScriptSound( "Weapon_R700_ZM.Zoom" );
}

void CZMWeaponR700::Equip( CBaseCombatCharacter* pCharacter )
{
    BaseClass::Equip( pCharacter );

    m_iZoomLevel = SCOPEZOOM_2X;
}

Vector CZMWeaponR700::GetBulletSpread() const
{
    Vector cone = CZMBaseWeapon::GetBulletSpread();

    CZMPlayer* pOwner = GetPlayerOwner();
    if ( pOwner )
    {
        float ratio;

        if ( !IsZoomed() )
        {
            ratio = 1.0f;
        }
        else
        {
            ratio = 1.0f - pOwner->GetAccuracyRatio();
            ratio *= ratio;
        }

        cone.x = ratio * cone.x;
        cone.y = ratio * cone.y;
        cone.z = ratio * cone.z;
    }


    return cone;
}

#ifdef CLIENT_DLL
float CZMWeaponR700::GetScopeFOVModifier() const
{
    float modifier;
    switch ( m_iZoomLevel )
    {
    case SCOPEZOOM_4X :
        modifier = 4.0f;
        break;
    case SCOPEZOOM_2X :
        modifier = 2.0f;
        break;
    case SCOPEZOOM_1X :
    default :
        modifier = 1.0f;
        break;
    }

    return 1.0f / modifier;
}
#endif

void CZMWeaponR700::ItemPostFrame()
{
    BaseClass::ItemPostFrame();


    auto* pOwner = GetPlayerOwner();
    if ( !pOwner ) return;


    bool bPressedAttack3 = pOwner->m_afButtonPressed & IN_ATTACK3;


    if ( bPressedAttack3 )
    {
        ChangeZoom();
    }
}

void CZMWeaponR700::ChangeZoom()
{
    m_iZoomLevel = m_iZoomLevel + 1;
    if ( m_iZoomLevel <= SCOPEZOOM_NONE || m_iZoomLevel >= SCOPEZOOM_MAX )
    {
        m_iZoomLevel = SCOPEZOOM_1X;
    }

#ifdef GAME_DLL
    CRecipientFilter filter;
    GetPlayerOwner()->GetMyRecipientFilter( filter );

    EmitSound( filter, entindex(), "Weapon_R700_ZM.Zoom" );
#endif
}
