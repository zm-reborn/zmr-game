#include "cbase.h"
#include "props.h"
#include "items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define CRATE_MODEL     "models/items/item_item_crate.mdl"

class CZMEntItemCrate : public CPhysicsProp
{
public:
    DECLARE_CLASS( CZMEntItemCrate, CPhysicsProp );
    DECLARE_DATADESC();

    void Precache();
    void Spawn();


    inline const char*  GetItemClass() { return STRING( m_strItemClass ); };
    inline int          GetItemCount() { return m_nItemCount; };

private:
    virtual int     ObjectCaps() OVERRIDE { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };

    virtual void    OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason ) OVERRIDE;
    virtual void    OnBreak( const Vector& vecVelocity, const AngularImpulse& angVel, CBaseEntity* pBreaker ) OVERRIDE;


    int             TranslateItemClassToSkin();


    string_t        m_strItemClass;
    int             m_nItemCount;

    COutputEvent    m_OnCacheInteraction;
};


LINK_ENTITY_TO_CLASS( item_item_crate, CZMEntItemCrate );


BEGIN_DATADESC( CZMEntItemCrate )
    DEFINE_KEYFIELD( m_strItemClass, FIELD_STRING, "ItemClass" ),
    DEFINE_KEYFIELD( m_nItemCount, FIELD_INTEGER, "ItemCount" ),

    DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),
END_DATADESC()


void CZMEntItemCrate::Precache()
{
    // Set this here to quiet base prop warnings
    PrecacheModel( CRATE_MODEL );

    BaseClass::Precache();

    
    if ( NULL_STRING != m_strItemClass )
    {
        // Don't precache if this is a null string. 
        UTIL_PrecacheOther( STRING( m_strItemClass ) );
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

    if (NULL_STRING == m_strItemClass
    ||  EntityFactoryDictionary()->FindFactory( STRING( m_strItemClass ) ) == nullptr ) // Is this an invalid classname?
    {
        Warning( "Invalid or no item class defined for item crate! (%i | item class: '%s')\n", entindex(), STRING( m_strItemClass ) );
        UTIL_Remove( this );
        return;
    }


    SetModelName( AllocPooledString( CRATE_MODEL ) );

    BaseClass::Spawn();
    Precache();

    DisableAutoFade();
    SetModel( CRATE_MODEL );
    AddEFlags( EFL_NO_ROTORWASH_PUSH );


    //if ( GetModelPtr() )
    //{
        //int nItemSkin = TranslateItemClassToSkin();

        //if ( nItemSkin > 0 && nItemSkin < GetModelPtr()->numskinfamilies() )
        //{
        //    m_nSkin = nItemSkin;
        //}
    //}
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
        CBaseEntity* pSpawn = CreateEntityByName( STRING( m_strItemClass ) );

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

void CZMEntItemCrate::OnPhysGunPickup( CBasePlayer* pPhysGunUser, PhysGunPickup_t reason )
{
    BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

    m_OnCacheInteraction.FireOutput( pPhysGunUser, this );
}

// strncmp for ammo since some of them use *_large classnames.
#define AMMO_EQU(str,classname)             ( Q_strncmp( str, classname, sizeof classname ) == 0 )

// Strict compare for weapon classnames.
#define WEAPON_EQU(str,classname)           ( Q_strcmp( str, classname ) == 0 )

int CZMEntItemCrate::TranslateItemClassToSkin()
{
    // ZMRTODO: Implement me.
    const char* c = STRING( m_strItemClass );


    if ( AMMO_EQU( c, "item_ammo_pistol" ) )
    {
        
    }
    if ( AMMO_EQU( c, "item_ammo_revolver" ) )
    {
        
    }
    if ( AMMO_EQU( c, "item_ammo_smg1" ) )
    {
        
    }
    if ( AMMO_EQU( c, "item_box_buckshot" ) )
    {
        
    }
    if ( AMMO_EQU( c, "item_ammo_357" ) )
    {
        
    }


    if ( WEAPON_EQU( c, "weapon_zm_pistol" ) )
    {
        
    }
    if ( WEAPON_EQU( c, "weapon_zm_revolver" ) )
    {
        
    }
    if ( WEAPON_EQU( c, "weapon_zm_mac10" ) )
    {
        
    }
    if ( WEAPON_EQU( c, "weapon_zm_shotgun" ) )
    {
        
    }
    if ( WEAPON_EQU( c, "weapon_zm_rifle" ) )
    {
        
    }
    if ( WEAPON_EQU( c, "weapon_zm_molotov" ) )
    {
        
    }


    return -1;
}
