#include "cbase.h"
#include "npcevent.h"
#include "fire.h"

#include "zmr_gamerules.h"
#include "zmr_immolator.h"



#define ZOMBIE_MODEL            "models/zombie/burnzie.mdl"


ConVar zm_sv_immolator_burndist( "zm_sv_immolator_burndist", "250", FCVAR_NOTIFY );

ConVar zm_sv_immolator_burndmgdist( "zm_sv_immolator_burndmgdist", "132", FCVAR_NOTIFY, "The distance at which we will hurt others when burning." );
ConVar zm_sv_immolator_burndmgtime( "zm_sv_immolator_burndmgtime", "8", FCVAR_NOTIFY, "The time those near me will burn for." );
ConVar zm_sv_immolator_burndmg( "zm_sv_immolator_burndmg", "1", FCVAR_NOTIFY );
ConVar zm_sv_immolator_burnhealthcap( "zm_sv_immolator_burnhealthcap", "1", FCVAR_NOTIFY, "The health cap at which the immolator will ignite the player." );
ConVar zm_sv_immolator_waterdmg( "zm_sv_immolator_waterdmg", "10", FCVAR_NOTIFY, "The damage we take every second from water." );

#define BURNOTHER_INTERVAL          1.0f


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


    m_flNextBurnDamageTime = 0.0f;
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

    if ( GetWaterLevel() >= WL_Waist )
    {
        float dmg = zm_sv_immolator_waterdmg.GetFloat() * GetUpdateInterval();
        
        CTakeDamageInfo dmgInfo( this, this, dmg, DMG_GENERIC );
        TakeDamage( dmgInfo );
    }


    // We're on fire, burn people around me.
    if ( IsOnFire() && m_flNextBurnDamageTime <= gpGlobals->curtime )
    {
        BurnHurtOthers();

        m_flNextBurnDamageTime = gpGlobals->curtime + BURNOTHER_INTERVAL;
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
    if ( GetWaterLevel() == WL_NotInWater )
    {
        StartFires();
    }
}

void CZMImmolator::BurnHurtOthers()
{
    CBaseEntity* pEnt = nullptr;

    CTraceFilterSimple filter( this, COLLISION_GROUP_NONE );
    trace_t tr;

    const Vector mypos = WorldSpaceCenter();

    const float flBurnTime = zm_sv_immolator_burndmgtime.GetFloat();
    
    while ( (pEnt = gEntList.FindEntityInSphere( pEnt, mypos, zm_sv_immolator_burndmgdist.GetFloat() )) != nullptr )
    {
        // They're in water
        if ( pEnt->GetWaterLevel() >= WL_Waist )
            continue;


        // Can't damage if we can't see.
        UTIL_TraceLine( mypos, pEnt->WorldSpaceCenter(), MASK_SOLID & ~(CONTENTS_MONSTER), &filter, &tr );

        if ( tr.fraction != 1.0f )
            continue;


        if ( pEnt->IsPlayer() || pEnt->IsBaseZombie() )
        {
            auto* pChar = static_cast<CBaseCombatCharacter*>( pEnt );

            if ( !pChar->IsOnFire() )
            {
                // Damage characters separately
                CTakeDamageInfo info( this, this, zm_sv_immolator_burndmg.GetFloat(), DMG_BURN );
                pEnt->TakeDamage( info );

                // Finish them off by burning them alive!
                if ( pEnt->GetHealth() <= zm_sv_immolator_burnhealthcap.GetInt() )
                {
                    pChar->Ignite( flBurnTime, false );
                }
            }
        }
        else
        {
            // Ignite props normally.
            auto* pBreak = dynamic_cast<CBreakableProp*>( pEnt );
            if ( pBreak )
            {
                pBreak->Ignite( flBurnTime, false );
            }
        }
    }
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

bool CZMImmolator::ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const
{
    //if ( iHitGroup == HITGROUP_HEAD )
    //{
    //    return BaseClass::ScaleDamageByHitgroup( iHitGroup, info );
    //}

    return false;
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
    EmitSound( "NPC_BurnZombie.Scream" );

    g_flLastZombieSound = gpGlobals->curtime;
}
