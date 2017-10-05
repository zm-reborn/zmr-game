#include "cbase.h"
#include "ammodef.h"

#include "zmr/zmr_shareddefs.h"
#include "zmr_ammo.h"

/*
    NOTE:

    You have to comment ammo classes in hl2/item_ammo.cpp
*/

int ITEM_GiveAmmo( CBasePlayer* pPlayer, float flCount, const char* pszAmmoName, bool bSuppressSound = false );


#define REGISTER_ZMAMMO(classname,classname2,model,ammoname,ammocount)  class classname2 : public CZMAmmo \
                                                                        { \
                                                                        public: \
                                                                            virtual const char* GetItemModel() OVERRIDE { return model; }; \
                                                                            virtual const char* GetAmmoName() OVERRIDE { return ammoname; }; \
                                                                            virtual int GetAmmoCount() OVERRIDE { return ammocount; }; \
                                                                        }; \
                                                                        LINK_ENTITY_TO_CLASS( classname, classname2 );

CZMAmmo::CZMAmmo()
{
    m_iAmmoType = -1;
}

void CZMAmmo::Spawn()
{
    Precache();
    SetModel( GetItemModel() );

    BaseClass::Spawn( );
}

void CZMAmmo::Precache()
{
    PrecacheModel( GetItemModel() );


    m_iAmmoType = GetAmmoDef()->Index( GetAmmoName() );
}

bool CZMAmmo::MyTouch( CBasePlayer* pPlayer )
{
    if ( ITEM_GiveAmmo( pPlayer, GetAmmoCount(), GetAmmoName() ) )
    {
        if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_NO )
            UTIL_Remove( this );

        return true;
    }

    return false;
}

IMPLEMENT_SERVERCLASS_ST( CZMAmmo, DT_ZM_Ammo )
    SendPropInt( SENDINFO( m_iAmmoType ), 8 ),
END_SEND_TABLE()


REGISTER_ZMAMMO( item_box_buckshot, CZMAmmo_Buckshot, "models/items/boxbuckshot.mdl", "Buckshot", SIZE_AMMO_BUCKSHOT );
REGISTER_ZMAMMO( item_ammo_357, CZMAmmo_357, "models/items/357ammo.mdl", "357", SIZE_AMMO_357 );
REGISTER_ZMAMMO( item_ammo_357_large, CZMAmmo_357Large, "models/items/357ammo.mdl", "357", SIZE_AMMO_357_LARGE );
REGISTER_ZMAMMO( item_ammo_smg1, CZMAmmo_Smg1, "models/items/boxmrounds.mdl", "SMG1", SIZE_AMMO_SMG1 );
REGISTER_ZMAMMO( item_ammo_smg1_large, CZMAmmo_Smg1Large, "models/items/boxmrounds.mdl", "SMG1", SIZE_AMMO_SMG1_LARGE );
REGISTER_ZMAMMO( item_ammo_pistol, CZMAmmo_Pistol, "models/items/boxsrounds.mdl", "Pistol", SIZE_AMMO_PISTOL );
REGISTER_ZMAMMO( item_ammo_pistol_large, CZMAmmo_PistolLarge, "models/items/boxsrounds.mdl", "Pistol", SIZE_AMMO_PISTOL_LARGE );
REGISTER_ZMAMMO( item_ammo_revolver, CZMAmmo_Revolver, "models/items/revolverammo.mdl", "Revolver", SIZE_ZMAMMO_REVOLVER );
