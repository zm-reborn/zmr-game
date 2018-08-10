#include "cbase.h"
#include "npcevent.h"
#include "fire.h"

#include "zmr_gamerules.h"
#include "zmr_immolator.h"



#define ZOMBIE_MODEL            "models/zombie/burnzie.mdl"


ConVar zm_sv_immolator_burndist( "zm_sv_immolator_burndist", "250", FCVAR_NOTIFY );


extern ConVar zm_sk_burnzombie_health;
extern ConVar zm_sk_burnzombie_dmg;

LINK_ENTITY_TO_CLASS( npc_burnzombie, CZMImmolator );
PRECACHE_REGISTER( npc_burnzombie );

IMPLEMENT_SERVERCLASS_ST( CZMImmolator, DT_ZM_Immolator )
END_SEND_TABLE()

CZMImmolator::CZMImmolator()
{
    SetZombieClass( ZMCLASS_IMMOLATOR );
    CZMRules::IncPopCount( GetZombieClass() );
}

CZMImmolator::~CZMImmolator()
{
}

void CZMImmolator::Precache()
{
    if ( !IsPrecacheAllowed() )
        return;


    PrecacheScriptSound( "Zombie.AttackHit" );
    PrecacheScriptSound( "Zombie.AttackMiss" );
    PrecacheScriptSound( "NPC_BurnZombie.Pain" );
    PrecacheScriptSound( "NPC_BurnZombie.Die" );
    PrecacheScriptSound( "NPC_BurnZombie.Alert" );
    PrecacheScriptSound( "NPC_BurnZombie.Idle" );
    //PrecacheScriptSound( "NPC_BurnZombie.Attack" ); // Don't have this
    PrecacheScriptSound( "NPC_BurnZombie.Scream" );


    BaseClass::Precache();
}

void CZMImmolator::Spawn()
{
    SetMaxHealth( zm_sk_burnzombie_health.GetInt() );


    BaseClass::Spawn();
}

void CZMImmolator::PreUpdate()
{
    BaseClass::PreUpdate();

    if ( !IsOnFire() && GetEnemy() )
    {
        float burndistsqr = zm_sv_immolator_burndist.GetFloat();
        burndistsqr *= burndistsqr;
        float flDist = GetAbsOrigin().DistToSqr( GetEnemy()->GetAbsOrigin() );
        if ( flDist < burndistsqr )
        {
            Ignite( 1337.0f );
        }
    }
}

void CZMImmolator::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
    if ( !IsOnFire() && IsAlive() )
    {
        //RemoveSpawnFlags( SF_NPC_GAG );

        EmitSound( "NPC_BurnZombie.Scream" );
    }

    BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );
}

void CZMImmolator::Event_Killed( const CTakeDamageInfo& info )
{
    BaseClass::Event_Killed( info );

    // We need to call this last or we will go on an endless loop
    StartFires();
}

void CZMImmolator::StartFires()
{
    const Vector vecStart = WorldSpaceCenter();
    Vector vecEnd;
    QAngle vecTraceAngles;
    Vector vecTraceDir;
    trace_t firetrace;

    for( int i = 0; i < 5; i++ )
    {
        // build a little ray
        vecTraceAngles[PITCH]   = random->RandomFloat( 45, 135 );
        vecTraceAngles[YAW]     = random->RandomFloat( 0, 360 );
        vecTraceAngles[ROLL]    = 0.0f;

        AngleVectors( vecTraceAngles, &vecTraceDir );

        //TGB: 	0000392, trace too long causing flames to appear on ground even when zombie is standing high up
        vecEnd = vecStart + vecTraceDir * 128; //TGB: was: 256

        UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &firetrace );

        Vector	ofsDir = ( firetrace.endpos - GetAbsOrigin() );
        float	offset = VectorNormalize( ofsDir );

        if ( offset > 128 )
            offset = 128;

        //Get our scale based on distance
        //float scale	 = 0.4f + ( 0.75f * ( 1.0f - ( offset / 128.0f ) ) );
        float growth = 0.8f + ( 0.75f * ( offset / 128.0f ) );
        float scale = 125.0f;

        if( firetrace.fraction != 1.0 )
        {
            FireSystem_StartFire( firetrace.endpos, scale, growth, 20.0f, (SF_FIRE_START_ON|SF_FIRE_SMOKELESS|SF_FIRE_NO_GLOW), nullptr, FIRE_NATURAL );
        }
    }
}

void CZMImmolator::HandleAnimEvent( animevent_t* pEvent )
{
    if (pEvent->event == AE_ZOMBIE_ATTACK_LEFT
    ||  pEvent->event == AE_ZOMBIE_ATTACK_RIGHT)
    {
        Vector right, forward;
        AngleVectors( GetLocalAngles(), &forward, &right, NULL );

        right = right * -100;
        forward = forward * 200;

        QAngle viewpunch( -15, 20, -10 );
        Vector punchvel = right + forward;

        ClawAttack( GetClawAttackRange(), zm_sk_burnzombie_dmg.GetInt(), viewpunch, punchvel );
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

bool CZMImmolator::ShouldPlayIdleSound() const
{
    /*
    return  BaseClass::ShouldPlayIdleSound()
    &&      GetEnemy() == nullptr // We must be idling.
    &&      random->RandomInt( 0, 120 ) == 0;
    */
    return false;
}

float CZMImmolator::IdleSound()
{
    // Immolator has a very weird idle sound.
    //EmitSound( "NPC_BurnZombie.Idle" );
    return 5.0f;
}

void CZMImmolator::AlertSound()
{
}

void CZMImmolator::DeathSound()
{
    EmitSound( "NPC_BurnZombie.Die" );
}
