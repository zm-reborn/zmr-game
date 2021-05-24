#include "cbase.h"
#include "props.h"
#include "items.h"
#include "TemplateEntities.h"
#include "mapentities.h"


#include "weapons/zmr_base.h"
#include "zmr_mapitemaction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


enum CrateModelType_t
{
    CRATEMODEL_NONE = -1,

    CRATEMODEL_AMMO,
    CRATEMODEL_MELEE,
    CRATEMODEL_THROWABLE,
    CRATEMODEL_WEAPON,

    CRATEMODEL_MAX
};

struct CrateItemData_t
{
    const char* pszItemName;
    CrateModelType_t iCrateModel;
    int iBodyGroup;
    int iSubModel;
    int iSkin;
};


const char* g_szCrateModels[] = {
    "models/items/crates/crate_ammo.mdl",
    "models/items/crates/crate_melee.mdl",
    "models/items/crates/crate_throw.mdl",
    "models/items/crates/crate_wepin.mdl"
};

const CrateItemData_t CrateItemData[] = {
    { "item_ammo_pistol", CRATEMODEL_AMMO, 0, 0, 1 },
    { "item_ammo_pistol_large", CRATEMODEL_AMMO, 0, 0, 1 },
    { "item_ammo_revolver", CRATEMODEL_AMMO, 0, 0, 2 },
    { "item_box_buckshot", CRATEMODEL_AMMO, 0, 0, 3 },
    { "item_ammo_357", CRATEMODEL_AMMO, 0, 0, 4 },
    { "item_ammo_357_large", CRATEMODEL_AMMO, 0, 0, 4 },
    { "item_ammo_smg1", CRATEMODEL_AMMO, 0, 0, 5 },
    { "item_ammo_smg1_large", CRATEMODEL_AMMO, 0, 0, 5 },

    { "weapon_zm_pistol", CRATEMODEL_WEAPON, 0, 0, 1 },
    { "weapon_zm_revolver", CRATEMODEL_WEAPON, 0, 0, 2 },
    { "weapon_zm_shotgun", CRATEMODEL_WEAPON, 0, 0, 3 },
    { "weapon_zm_rifle", CRATEMODEL_WEAPON, 0, 0, 4 },
    { "weapon_zm_mac10", CRATEMODEL_WEAPON, 0, 0, 5 },

    { "weapon_zm_fireaxe", CRATEMODEL_MELEE, 1, 0, 0 },
    { "weapon_zm_improvised", CRATEMODEL_MELEE, 1, 1, 0 },
    { "weapon_zm_sledge", CRATEMODEL_MELEE, 1, 2, 0 },

    { "weapon_zm_molotov", CRATEMODEL_THROWABLE, 0, 0, 0 },
    { "weapon_zm_pipebomb", CRATEMODEL_THROWABLE, 0, 0, 0 },
};

#define CRATE_MODEL_OLD                     "models/items/item_item_crate.mdl"

class CZMEntItemCrate : public CPhysicsProp
{
public:
    DECLARE_CLASS( CZMEntItemCrate, CPhysicsProp );
    DECLARE_DATADESC();

    CZMEntItemCrate();

    void Precache();
    void Spawn();


    static CrateModelType_t TranslateItemClassNameToType( const char* pszItemClassName );
    static int              TranslateItemClassToSkin( const char* pszItemClassName, int& iBodyGroup, int& iSubModel );

    inline const char*  GetItemClass() const { return STRING( m_iszItemClass ); };
    inline int          GetItemCount() const { return m_nItemCount; };

private:
    virtual int     ObjectCaps() OVERRIDE { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };

    virtual void    OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason ) OVERRIDE;
    virtual void    OnBreak( const Vector& vecVelocity, const AngularImpulse& angVel, CBaseEntity* pBreaker ) OVERRIDE;

    CBaseEntity*    CreateItem() const;
    CZMBaseWeapon*  CreateTemplateItem() const;



    string_t        m_iszItemClass;
    int             m_nItemCount;

    string_t        m_iszTemplateName;
    string_t        m_iszTemplateData;


    COutputEvent    m_OnCacheInteraction;
};


LINK_ENTITY_TO_CLASS( item_item_crate, CZMEntItemCrate );
PRECACHE_REGISTER( item_item_crate );


BEGIN_DATADESC( CZMEntItemCrate )
    DEFINE_KEYFIELD( m_iszItemClass, FIELD_STRING, "ItemClass" ),
    DEFINE_KEYFIELD( m_nItemCount, FIELD_INTEGER, "ItemCount" ),

    DEFINE_KEYFIELD( m_iszTemplateName, FIELD_STRING, "ItemTemplate" ),


    DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),
END_DATADESC()


CZMEntItemCrate::CZMEntItemCrate()
{
    m_iszItemClass = NULL_STRING;
    m_iszTemplateName = NULL_STRING;
    m_iszTemplateData = NULL_STRING;
}

void CZMEntItemCrate::Precache()
{
    Assert( ARRAYSIZE( g_szCrateModels ) == CRATEMODEL_MAX );
    for ( int i = 0; i < ARRAYSIZE( g_szCrateModels ); i++ )
    {
        PrecacheModel( g_szCrateModels[i] );
        
    }

    BaseClass::Precache();

    
    if ( m_iszItemClass != NULL_STRING )
    {
        UTIL_PrecacheOther( STRING( m_iszItemClass ) );
    }


    // We have template data, precache our template entity.
	if ( m_iszTemplateData != NULL_STRING )
	{
        CBaseEntity* pEntity = CreateTemplateItem();

        if ( pEntity )
        {
            pEntity->Precache();
            UTIL_RemoveImmediate( pEntity );
        }
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMEntItemCrate::Spawn()
{ 
    if ( !g_pGameRules->IsAllowedToSpawn( this ) )
    {
        UTIL_Remove( this );
        return;
    }

    if ( m_nItemCount < 1 )
    {
        Warning( "Item crate has no items! (%i | item count: %i)\n", entindex(), m_nItemCount );
        UTIL_Remove( this );
        return;
    }


    // Attempt to use template first.
    if ( m_iszTemplateName != NULL_STRING )
    {
        CBaseEntity* pTestEntity = nullptr;

        bool bFailed = false;


        if ((m_iszTemplateData = Templates_FindByTargetName( STRING( m_iszTemplateName ) )) == NULL_STRING // Is it a template?
        ||  (pTestEntity = CreateTemplateItem()) == nullptr) // Attempt to create the template.
        {
            Warning( "Invalid or no item template defined for item crate! (%i | item template: '%s')\n", entindex(), STRING( m_iszTemplateName ) );
            UTIL_Remove( this );
            
            bFailed = true;
        }

        if ( pTestEntity )
            UTIL_RemoveImmediate( pTestEntity );


        if ( bFailed ) return;
    }
    else if (NULL_STRING == m_iszItemClass
    ||  EntityFactoryDictionary()->FindFactory( STRING( m_iszItemClass ) ) == nullptr ) // Is this an invalid classname?
    {
        Warning( "Invalid or no item class defined for item crate! (%i | item class: '%s')\n", entindex(), STRING( m_iszItemClass ) );
        UTIL_Remove( this );
        return;
    }


    int iBodyGroup = 0;
    int iSubModel = 0;
    int iSkin = 0;

    auto iCrateType = TranslateItemClassNameToType( STRING( m_iszItemClass ) );
    iSkin = TranslateItemClassToSkin( STRING( m_iszItemClass ), iBodyGroup, iSubModel );

    const char* pszModelName;

    if ( iCrateType > CRATEMODEL_NONE && iCrateType < CRATEMODEL_MAX )
    {
        pszModelName = g_szCrateModels[iCrateType];
    }
    else
    {
        pszModelName = g_szCrateModels[0];
    }

    SetModelName( AllocPooledString( pszModelName ) );

    BaseClass::Spawn();
    Precache();

    DisableAutoFade();
    SetModel( pszModelName );
    AddEFlags( EFL_NO_ROTORWASH_PUSH );

    // Unfortunately, due to the model changes, we need to wake em up 
    // to settle them to the ground.
    auto* pPhys = VPhysicsGetObject();
    if ( pPhys && pPhys->IsAsleep() )
    {
        pPhys->Wake();
    }


    // Set skin based on item we house.
    auto* pHdr = GetModelPtr();
    if ( pHdr )
    {
        if ( iBodyGroup >= 0 && iBodyGroup < pHdr->numbodyparts() )
        {
            SetBodygroup( iBodyGroup, iSubModel );
        }
        

        if ( iSkin >= 0 && iSkin < pHdr->numskinfamilies() )
        {
            m_nSkin = iSkin;
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMEntItemCrate::OnBreak( const Vector& vecVelocity, const AngularImpulse& angImpulse, CBaseEntity* pBreaker )
{
    // FIXME: We could simply store the name of an entity to put into the crate 
    // as a string entered in by worldcraft. Should we?	I'd do it for sure
    // if it was easy to get a dropdown with all entity types in it.

    m_OnCacheInteraction.FireOutput( pBreaker, this );

    for ( int i = 0; i < m_nItemCount; ++i )
    {
        CBaseEntity* pSpawn = CreateItem();

        if ( !pSpawn )
            return;


        // Give a little randomness...
        Vector vecOrigin;
        CollisionProp()->RandomPointInBounds( Vector(0.25, 0.25, 0.25), Vector( 0.75, 0.75, 0.75 ), &vecOrigin );
        pSpawn->SetAbsOrigin( vecOrigin );

        QAngle vecAngles;
        vecAngles.x = random->RandomFloat( -20.0f, 20.0f );
        vecAngles.y = random->RandomFloat( 0.0f, 360.0f );
        vecAngles.z = random->RandomFloat( -20.0f, 20.0f );
        pSpawn->SetAbsAngles( vecAngles );

        Vector vecActualVelocity;
        vecActualVelocity.Random( -10.0f, 10.0f );
        //vecActualVelocity += vecVelocity;
        pSpawn->SetAbsVelocity( vecActualVelocity );

        QAngle angVel;
        AngularImpulseToQAngle( angImpulse, angVel );
        pSpawn->SetLocalAngularVelocity( angVel );

        // ZMRCHANGE: Fixes not being able to pickup items from item_item_crate.
        CItem* pItem = dynamic_cast<CItem*>( pSpawn );

        if ( pItem ) pItem->Spawn();
        else pSpawn->Spawn();
    }
}

CBaseEntity* CZMEntItemCrate::CreateItem() const
{
    // It's a template, spawn that.
    if ( m_iszTemplateData != NULL_STRING )
    {
        return CreateTemplateItem();
    }
    
    //
    // Check if we should spawn other types of this item.
    // Eg. weapon_zm_shotgun and weapon_zm_shotgun_sporting
    //
    auto* classname = STRING( m_iszItemClass );


    auto flags = ZMItemAction::g_ZMMapItemSystem.GetItemFlags( classname );

    // It's a weapon.
    if ( flags & ZMItemAction::CLASS_WEAPON )
    {
        // Collect the possible weapons we could give.
        CUtlVector<const ZMItemAction::ItemBaseData_t*> items;
        ZMItemAction::CZMMapItemSystem::GetMapItems( [ flags ]( const ZMItemAction::ItemBaseData_t& itemdata )
        {
            return itemdata.m_fClassFlags == flags;
        }, items );

        if ( items.Count() > 1 )
        {
            return CreateEntityByName( items[random->RandomInt( 0, items.Count() - 1 )]->m_pszClassname );
        }
    }

    return CreateEntityByName( classname );
}

CZMBaseWeapon* CZMEntItemCrate::CreateTemplateItem() const
{
    CBaseEntity* pEntity = nullptr;

    MapEntity_ParseEntity( pEntity, STRING( m_iszTemplateData ), nullptr );

    if ( !pEntity )
    {
        return nullptr;
    }

    CZMBaseWeapon* pWeapon = ToZMBaseWeapon( pEntity );

    if ( !pWeapon )
    {
        UTIL_RemoveImmediate( pEntity );
    }
    else
    {
        pWeapon->RemoveSpawnFlags( SF_ZMWEAPON_TEMPLATE );
    }

    return pWeapon;
}

void CZMEntItemCrate::OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason )
{
    BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

    m_OnCacheInteraction.FireOutput( pPhysGunUser, this );
}

// strncmp for ammo since some of them use *_large classnames.
#define AMMO_EQU(str,classname)             ( Q_strncmp( str, classname, sizeof( classname ) - 1 ) == 0 )

// Strict compare for weapon classnames.
#define WEAPON_EQU(str,classname)           ( Q_strcmp( str, classname ) == 0 )


CrateModelType_t CZMEntItemCrate::TranslateItemClassNameToType( const char* pszItemClassName )
{
    Assert( pszItemClassName );

    for ( int i = 0; i < ARRAYSIZE( CrateItemData ); i++ )
    {
        if ( Q_strcmp( pszItemClassName, CrateItemData[i].pszItemName ) == 0 )
        {
            return CrateItemData[i].iCrateModel;
        }
    }

    return CRATEMODEL_NONE;
}

// Return -1 for invalid
int CZMEntItemCrate::TranslateItemClassToSkin( const char* pszItemClassName, int& iBodyGroup, int &iSubModel )
{
    iBodyGroup = 0;
    iSubModel = 0;

    for ( int i = 0; i < ARRAYSIZE( CrateItemData ); i++ )
    {
        if ( Q_strcmp( pszItemClassName, CrateItemData[i].pszItemName ) == 0 )
        {
            iBodyGroup = CrateItemData[i].iBodyGroup;
            iSubModel = CrateItemData[i].iSubModel;
            return CrateItemData[i].iSkin;
        }
    }

    return -1;
}
