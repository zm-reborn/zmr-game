#include "cbase.h"
#include "basegrenade_shared.h"
#include "gib.h"
#include "props_shared.h"
#include "smoke_Trail.h"
#include "fire.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MOLOTOV_MODEL       "models/weapons/molotov3rd_zm.mdl"


//
//
//
class CFlamingGib : public CGib
{
public:
    DECLARE_CLASS( CFlamingGib, CGib );
    
    virtual void Spawn() OVERRIDE;
};

LINK_ENTITY_TO_CLASS( flaming_gib, CFlamingGib );

void CFlamingGib::Spawn()
{
    const char* gibname = g_PropDataSystem.GetRandomChunkModel( "MetalChunks" );

    BaseClass::Spawn( gibname );


    SetBloodColor( DONT_BLEED );
    m_takedamage = DAMAGE_NO;

    SetCollisionGroup( COLLISION_GROUP_DEBRIS );
    AddEffects( EF_NODRAW );
        




    SetNextThink( gpGlobals->curtime + m_lifeTime );
    SetThink( &CGib::DieThink );
        


    QAngle ang;
    Vector vel;
    ang.x = random->RandomFloat( -70.0f, 20.0f );
    ang.y = random->RandomFloat( 0.0f, 360.0f );
    ang.z = 0;
    AngleVectors( ang, &vel );

    vel *= random->RandomFloat( 10.0f, 20.0f );

    SetAbsVelocity( vel );


    IPhysicsObject* pPhys = VPhysicsInitNormal( SOLID_NONE, GetSolidFlags(), false );

    if ( pPhys )
    {
        AngularImpulse angimp = RandomAngularImpulse( -90.0f, 90.0f );



        pPhys->EnableMotion( true );
        pPhys->SetVelocity(
            &vel,
            &angimp );
    }

    CFireTrail* trail = CFireTrail::CreateFireTrail();

    if ( trail )
    {
        trail->FollowEntity( this, "" );
        trail->SetParent( this, 0 );
        trail->SetLocalOrigin( vec3_origin );
        trail->SetMoveType( MOVETYPE_NONE );
        trail->SetLifetime( this->m_lifeTime - 1.0f );
        //trail->m_StartSize = 15; //TGB: made the chunks smaller
        //trail->m_EndSize = 6;  
    }
}

//
//
//
class CZMProjectileMolotov : public CBaseGrenade
{
public:
    DECLARE_CLASS( CZMProjectileMolotov, CBaseGrenade );
    DECLARE_DATADESC();


    void Spawn() OVERRIDE;
    void Precache() OVERRIDE;

    void MolotovTouch( CBaseEntity* pOther );
    void MolotovThink();
    void Detonate() OVERRIDE;

    void CreateFlyingChunk( const Vector& pos );

private:
    SmokeTrail* m_pFireTrail;
};

#ifndef CLIENT_DLL
BEGIN_DATADESC( CZMProjectileMolotov )
    DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),

    DEFINE_ENTITYFUNC( MolotovTouch ),
    DEFINE_THINKFUNC( MolotovThink ),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( grenade_molotov, CZMProjectileMolotov );

void CZMProjectileMolotov::Precache()
{
    PrecacheModel( MOLOTOV_MODEL );

    PrecacheScriptSound( "Grenade_Molotov.Detonate" );
    PrecacheScriptSound( "Grenade_Molotov.Detonate2" );
}

void CZMProjectileMolotov::Spawn()
{
    Precache();

    SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
    SetSolid( SOLID_BBOX ); 
    SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
    RemoveEffects( EF_NOINTERP );

    SetModel( MOLOTOV_MODEL );


    SetTouch( &CZMProjectileMolotov::MolotovTouch );
    SetThink( &CZMProjectileMolotov::MolotovThink );


    CFireTrail* fire = CFireTrail::CreateFireTrail();

    if ( fire )
    {
        fire->FollowEntity( this, "flame" );
        fire->SetLocalOrigin( vec3_origin );
        fire->SetMoveType( MOVETYPE_NONE );
        fire->SetLifetime( 20.0f ); 
        //fire->m_StartSize = random->RandomFloat(1.0f, 2.0f);
        //fire->m_EndSize = random->RandomFloat(3.5f, 5.0f);
    }
}

void CZMProjectileMolotov::MolotovTouch( CBaseEntity* pOther )
{
    if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS ) )
        return;


    Detonate();
}

void CZMProjectileMolotov::MolotovThink()
{
    if ( UTIL_PointContents( GetAbsOrigin() ) & CONTENTS_WATER )
    {
        UTIL_Remove( this );
        return;
    }

    SetNextThink( gpGlobals->curtime + 0.1f );
}

void CZMProjectileMolotov::Detonate()
{
    AddEffects( EF_NODRAW );


    // Travel back to the touch position.
    trace_t trace;
    trace = GetTouchTrace();

    Vector normal( 0.0f, 0.0f, 1.0f );

    if ( trace.fraction != 1.0f )
    {
        normal = trace.plane.normal;

        // Go off the wall a bit.
        SetAbsOrigin( trace.endpos + normal * 2.0f );
    }
    

    if ( UTIL_PointContents( GetAbsOrigin() ) & CONTENTS_WATER )
    {
        UTIL_Remove( this );
        return;
    }


    EmitSound( "Grenade_Molotov.Detonate" );
    EmitSound( "Grenade_Molotov.Detonate2" );

    // ZMRTODO: Fix flying chunks.
    //for ( int i = 0; i < 5; i++ )
    //{
    //    CreateFlyingChunk( GetAbsOrigin() );
    //}

    CBaseCombatCharacter* pThrower = GetThrower();

    Vector start, end;
    Vector fwd;
    QAngle ang;

    trace_t firetrace;
    start = GetAbsOrigin() + normal * 48.0f;
    for ( int i = 0; i < 5; i++ )
    {
        ang.x = random->RandomFloat( 45.0f, 135.0f );
        ang.y = random->RandomFloat( 0.0f, 360.0f );
        ang.z = 0.0f;

        AngleVectors( ang, &fwd );

        end = start + fwd * 300.0f;


        UTIL_TraceLine( start, end, MASK_SOLID, this, COLLISION_GROUP_NONE, &firetrace );

        if ( firetrace.fraction != 1.0f )
        {
            FireSystem_StartFire( firetrace.endpos + ( firetrace.plane.normal * 12.0f ), 125.0f, 128.0f, 10.0f, (SF_FIRE_START_ON), pThrower, FIRE_NATURAL );
        }
        else
        {
            FireSystem_StartFire( firetrace.endpos, 125.0f, 128.0f, 20.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS), pThrower, FIRE_NATURAL );
        }
    }


    UTIL_ScreenShake( GetAbsOrigin(), 10.0f, 60.0f, 1.0f, 200.0f, SHAKE_START, true );


    if ( pThrower )
    {
        RadiusDamage( CTakeDamageInfo( this, pThrower, 40.0f, DMG_BURN ), GetAbsOrigin(), 128.0f, CLASS_NONE, NULL );
    }


    UTIL_Remove( this );
}

void CZMProjectileMolotov::CreateFlyingChunk( const Vector& pos )
{
    CFlamingGib* pGib = CREATE_ENTITY( CFlamingGib, "flaming_gib" );

    if ( !pGib )
    {
        Warning( "Couldn't create molotov gib!\n" );
        return;
    }

    pGib->m_lifeTime = 15.0f;
    pGib->Spawn();

    pGib->SetAbsOrigin( pos );
    pGib->SetOwnerEntity( this );
}
