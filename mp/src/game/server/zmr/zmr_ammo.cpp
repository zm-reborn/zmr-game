#include "cbase.h"
#include "ammodef.h"

#include "zmr_shareddefs.h"
#include "zmr_ammo.h"

/*
    NOTE:

    You have to comment ammo classes in hl2/item_ammo.cpp
*/

int ITEM_GiveAmmo( CBasePlayer* pPlayer, float flCount, const char* pszAmmoName, bool bSuppressSound = false );


#define REGISTER_ZMAMMO(classname,classname2,model,ammoname,ammocount)  class classname2 : public CZMAmmo \
                                                                        { \
                                                                        public: \
                                                                            classname2() { m_nMaxAmmo = ammocount; m_nAmmo = m_nMaxAmmo; } \
                                                                            virtual const char* GetItemModel() const OVERRIDE { return model; } \
                                                                            virtual const char* GetAmmoName() const OVERRIDE { return ammoname; } \
                                                                        }; \
                                                                        LINK_ENTITY_TO_CLASS( classname, classname2 );

CZMAmmo::CZMAmmo()
{
    m_iAmmoType = -1;
    m_nAmmo = 0;

    m_bInThrow = false;
}

void CZMAmmo::Spawn()
{
    Precache();
    SetModel( GetItemModel() );

    BaseClass::Spawn();
}

void CZMAmmo::SetNextPickupTouch( float delay )
{
    SetTouch( &CZMAmmo::EmptyTouch );

    SetThink( &CZMAmmo::NoPickupThink );
    SetNextThink( gpGlobals->curtime + delay );

    m_bInThrow = true;
}

void CZMAmmo::NoPickupThink()
{
    SetThink( nullptr );
    SetTouch( &CItem::ItemTouch );

    m_bInThrow = false;
}

void CZMAmmo::EmptyTouch( CBaseEntity* pOther )
{
}

void CZMAmmo::Precache()
{
    PrecacheModel( GetItemModel() );


    m_iAmmoType = GetAmmoDef()->Index( GetAmmoName() );
}

void CZMAmmo::OnEntityEvent( EntityEvent_t event, void* pEventData )
{
    // Ignore this event for CItem, as it fucks up our stuff, making ammo unpickable.
    if ( m_bInThrow && event == ENTITY_EVENT_WATER_TOUCH )
    {
        CBaseAnimating::OnEntityEvent( event, pEventData );
    }
    else
    {
        BaseClass::OnEntityEvent( event, pEventData );
    }
}

bool CZMAmmo::MyTouch( CBasePlayer* pPlayer )
{
    int given = ITEM_GiveAmmo( pPlayer, m_nAmmo, GetAmmoName() );
    if ( given )
    {
        m_nAmmo -= given;

        return ( m_nAmmo <= 0 );
    }

    return false;
}

IMPLEMENT_SERVERCLASS_ST( CZMAmmo, DT_ZM_Ammo )
    SendPropInt( SENDINFO( m_iAmmoType ), 8 ),
    SendPropInt( SENDINFO( m_nAmmo ), 8, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_nMaxAmmo ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMAmmo )
    DEFINE_ENTITYFUNC( EmptyTouch ),
    DEFINE_THINKFUNC( NoPickupThink ),
END_DATADESC()



REGISTER_ZMAMMO( item_box_buckshot, CZMAmmo_Buckshot, "models/items/boxbuckshot.mdl", "Buckshot", SIZE_AMMO_BUCKSHOT );
REGISTER_ZMAMMO( item_ammo_357, CZMAmmo_357, "models/items/357ammo.mdl", "357", SIZE_AMMO_357 );
REGISTER_ZMAMMO( item_ammo_357_large, CZMAmmo_357Large, "models/items/357ammo.mdl", "357", SIZE_AMMO_357_LARGE );
REGISTER_ZMAMMO( item_ammo_smg1, CZMAmmo_Smg1, "models/items/boxmrounds.mdl", "SMG1", SIZE_AMMO_SMG1 );
REGISTER_ZMAMMO( item_ammo_smg1_large, CZMAmmo_Smg1Large, "models/items/boxmrounds.mdl", "SMG1", SIZE_AMMO_SMG1_LARGE );
REGISTER_ZMAMMO( item_ammo_pistol, CZMAmmo_Pistol, "models/items/boxsrounds.mdl", "Pistol", SIZE_AMMO_PISTOL );
REGISTER_ZMAMMO( item_ammo_pistol_large, CZMAmmo_PistolLarge, "models/items/boxsrounds.mdl", "Pistol", SIZE_AMMO_PISTOL_LARGE );
REGISTER_ZMAMMO( item_ammo_revolver, CZMAmmo_Revolver, "models/items/revolverammo.mdl", "Revolver", SIZE_ZMAMMO_REVOLVER );
