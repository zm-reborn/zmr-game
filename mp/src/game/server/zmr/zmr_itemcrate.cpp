#include "cbase.h"
#include "props.h"
#include "items.h"
#include "TemplateEntities.h"
#include "mapentities.h"


#include "weapons/zmr_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define CRATE_MODEL     "models/items/item_item_crate.mdl"

class CZMEntItemCrate : public CPhysicsProp
{
public:
    DECLARE_CLASS( CZMEntItemCrate, CPhysicsProp );
    DECLARE_DATADESC();

    CZMEntItemCrate();

    void Precache();
    void Spawn();


    inline const char*  GetItemClass() const { return STRING( m_iszItemClass ); };
    inline int          GetItemCount() const { return m_nItemCount; };

private:
    virtual int     ObjectCaps() OVERRIDE { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };

    virtual void    OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason ) OVERRIDE;
    virtual void    OnBreak( const Vector& vecVelocity, const AngularImpulse& angVel, CBaseEntity* pBreaker ) OVERRIDE;


    int             TranslateItemClassToSkin();

    CBaseEntity*    CreateItem() const;
    CZMBaseWeapon*  CreateTemplateItem() const;



    string_t        m_iszItemClass;
    int             m_nItemCount;

    string_t        m_iszTemplateName;
    string_t        m_iszTemplateData;


    COutputEvent    m_OnCacheInteraction;
};


LINK_ENTITY_TO_CLASS( item_item_crate, CZMEntItemCrate );


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
    // Set this here to quiet base prop warnings
    PrecacheModel( CRATE_MODEL );

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


    SetModelName( AllocPooledString( CRATE_MODEL ) );

    BaseClass::Spawn();
    Precache();

    DisableAutoFade();
    SetModel( CRATE_MODEL );
    AddEFlags( EFL_NO_ROTORWASH_PUSH );


    // Set skin based on item we house.
    if ( GetModelPtr() )
    {
        int iItemSkin = TranslateItemClassToSkin();

        if ( iItemSkin >= 0 && iItemSkin < GetModelPtr()->numskinfamilies() )
        {
            m_nSkin = iItemSkin;
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
    return ( m_iszTemplateData != NULL_STRING ) ? CreateTemplateItem() : CreateEntityByName( STRING( m_iszItemClass ) );
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


// Return -1 for invalid
int CZMEntItemCrate::TranslateItemClassToSkin()
{
    // ZMRTODO: Also check template ones.
    const char* c = STRING( m_iszItemClass );

    bool bIsWeapon = c[0] == 'w';


    if ( bIsWeapon )
    {
        // Skip weapon_zm_
        int skip = sizeof( "weapon_zm_" ) - 1;

        int len = Q_strlen( c );
        if ( len <= skip )
            return -1;

        c += skip;


        if ( WEAPON_EQU( c, "molotov" ) )
        {
            return 1;
        }
        if ( WEAPON_EQU( c, "sledge" ) )
        {
            return 2;
        }
        if ( WEAPON_EQU( c, "improvised" ) )
        {
            return 3;
        }

        if ( WEAPON_EQU( c, "pistol" ) )
        {
            return 9;
        }
        if ( WEAPON_EQU( c, "revolver" ) )
        {
            return 10;
        }
        if ( AMMO_EQU( c, "shotgun" ) ) // shotgun && shotgun_sporting
        {
            return 11;
        }
        if ( WEAPON_EQU( c, "rifle" ) )
        {
            return 12;
        }
        if ( WEAPON_EQU( c, "mac10" ) )
        {
            return 13;
        }
    }
    else
    {
        if ( AMMO_EQU( c, "item_ammo_pistol" ) )
        {
            return 4;
        }
        if ( AMMO_EQU( c, "item_ammo_revolver" ) )
        {
            return 5;
        }
        if ( AMMO_EQU( c, "item_box_buckshot" ) )
        {
            return 6;
        }
        if ( AMMO_EQU( c, "item_ammo_357" ) )
        {
            return 7;
        }
        if ( AMMO_EQU( c, "item_ammo_smg1" ) )
        {
            return 8;
        }
    }


    return -1;
}
