#include "cbase.h"
#include "npcevent.h"
#include "activitylist.h"
#include "eventlist.h"

#include "zmr_gamerules.h"
#include "zmr/npcs/zmr_zombiebase.h"
#include "zmr_banshee.h"
#include "zmr/npcs/sched/zmr_zombie_banshee_ceil_ambush.h"
#include "zmr/npcs/sched/zmr_zombie_banshee_leap.h"


ConVar zm_sv_debug_bansheerangeattack( "zm_sv_debug_bansheerangeattack", "0" );


#define ZOMBIE_MODEL        "models/zombie/zm_fast.mdl"


ConVar zm_sv_banshee_leapdist_min( "zm_sv_banshee_leapdist_min", "200", FCVAR_NOTIFY );
ConVar zm_sv_banshee_leapdist_max( "zm_sv_banshee_leapdist_max", "300", FCVAR_NOTIFY );

ConVar zm_sv_banshee_ceilambush_detectrange( "zm_sv_banshee_ceilambush_detectrange", "256", FCVAR_NOTIFY, "", true, 0.0f, false, 0.0f );
ConVar zm_sv_banshee_ceilambush_maxheight( "zm_sv_banshee_ceilambush_maxheight", "375", FCVAR_NOTIFY, "", true, 0.0f, false, 0.0f );


LINK_ENTITY_TO_CLASS( npc_fastzombie, CZMBanshee );
PRECACHE_REGISTER( npc_fastzombie );


Activity CZMBanshee::ACT_FASTZOMBIE_FRENZY = ACT_INVALID;
Activity CZMBanshee::ACT_FASTZOMBIE_BIG_SLASH = ACT_INVALID;

Activity CZMBanshee::ACT_FASTZOMBIE_LAND_RIGHT = ACT_INVALID;
Activity CZMBanshee::ACT_FASTZOMBIE_LAND_LEFT = ACT_INVALID;
Activity CZMBanshee::ACT_FASTZOMBIE_LEAP_STRIKE = ACT_INVALID;

int CZMBanshee::AE_FASTZOMBIE_GALLOP_LEFT = AE_INVALID;
int CZMBanshee::AE_FASTZOMBIE_GALLOP_RIGHT = AE_INVALID;
int CZMBanshee::AE_FASTZOMBIE_LEAP = AE_INVALID;

extern ConVar zm_sk_banshee_dmg_claw;
extern ConVar zm_sk_banshee_health;


IMPLEMENT_SERVERCLASS_ST( CZMBanshee, DT_ZM_Banshee )
END_SEND_TABLE()

CZMBanshee::CZMBanshee()
{
    SetZombieClass( ZMCLASS_BANSHEE );
    CZMRules::IncPopCount( GetZombieClass() );


    m_flNextLeapAttack = 0.0f;


    m_pLeapSched = new BansheeLeapSched;
    m_pCeilAmbushSched = new BansheeCeilAmbushSched;
}

CZMBanshee::~CZMBanshee()
{
    delete m_pLeapSched;
    delete m_pCeilAmbushSched;
}

void CZMBanshee::Precache()
{
    if ( !IsPrecacheAllowed() )
        return;


    PrecacheModel( ZOMBIE_MODEL );

    PrecacheScriptSound( "NPC_FastZombie.LeapAttack" );
    PrecacheScriptSound( "NPC_FastZombie.FootstepRight" );
    PrecacheScriptSound( "NPC_FastZombie.FootstepLeft" );
    PrecacheScriptSound( "NPC_FastZombie.AttackHit" );
    PrecacheScriptSound( "NPC_FastZombie.AttackMiss" );
    PrecacheScriptSound( "NPC_FastZombie.LeapAttack" );
    PrecacheScriptSound( "NPC_FastZombie.Attack" );
    PrecacheScriptSound( "NPC_FastZombie.Idle" );
    PrecacheScriptSound( "NPC_FastZombie.AlertFar" );
    PrecacheScriptSound( "NPC_FastZombie.AlertNear" );
    PrecacheScriptSound( "NPC_FastZombie.GallopLeft" );
    PrecacheScriptSound( "NPC_FastZombie.GallopRight" );
    PrecacheScriptSound( "NPC_FastZombie.Scream" );
    PrecacheScriptSound( "NPC_FastZombie.RangeAttack" );
    PrecacheScriptSound( "NPC_FastZombie.Frenzy" );
    PrecacheScriptSound( "NPC_FastZombie.NoSound" );
    PrecacheScriptSound( "NPC_FastZombie.Die" );

    PrecacheScriptSound( "NPC_FastZombie.Gurgle" );

    PrecacheScriptSound( "NPC_FastZombie.Moan1" );



    REGISTER_PRIVATE_ACTIVITY( ACT_FASTZOMBIE_FRENZY );
    REGISTER_PRIVATE_ACTIVITY( ACT_FASTZOMBIE_BIG_SLASH );

    REGISTER_PRIVATE_ACTIVITY( ACT_FASTZOMBIE_LAND_RIGHT );
    REGISTER_PRIVATE_ACTIVITY( ACT_FASTZOMBIE_LAND_LEFT );
    REGISTER_PRIVATE_ACTIVITY( ACT_FASTZOMBIE_LEAP_STRIKE );


    REGISTER_PRIVATE_ANIMEVENT( AE_FASTZOMBIE_GALLOP_LEFT );
    REGISTER_PRIVATE_ANIMEVENT( AE_FASTZOMBIE_GALLOP_RIGHT );
    REGISTER_PRIVATE_ANIMEVENT( AE_FASTZOMBIE_LEAP );
    


    BaseClass::Precache();
}

void CZMBanshee::Spawn()
{
    SetModel( ZOMBIE_MODEL );

    SetMaxHealth( zm_sk_banshee_health.GetInt() );


    BaseClass::Spawn();
}

NPCR::CPathCostGroundOnly* CZMBanshee::GetPathCost() const
{
    static NPCR::CPathCostGroundOnly* cost = nullptr;
    if ( !cost )
    {
        const float MAX_JUMP_RISE       = 512.0f; // 220.0f
        //const float MAX_JUMP_DISTANCE   = 512.0f;
        //const float MAX_JUMP_DROP       = 1024.0f; // 384.0f

        cost = new NPCR::CPathCostGroundOnly;
        cost->SetMaxHeightChange( MAX_JUMP_RISE );
    }

    return cost;
}

void CZMBanshee::OnNavJump()
{
    //SetActivity( ACT_JUMP );

    BaseClass::OnNavJump();
}

bool CZMBanshee::IsAttacking() const
{
    Activity act = GetActivity();

    if ( act == ACT_FASTZOMBIE_BIG_SLASH )
        return true;
    if ( act == ACT_FASTZOMBIE_FRENZY )
        return true;

    return BaseClass::IsAttacking();
}

void CZMBanshee::StartCeilingAmbush()
{
    GetCommandQueue()->QueueCommand( new CZMCommandCeilingAmbush );
    OnQueuedCommand( COMMAND_CEILINGAMBUSH );
}

bool CZMBanshee::LeapAttack( const QAngle& angPunch, const Vector& vecPunchVel, float flDamage )
{
    Vector mins, maxs;
    GetAttackHull( mins, maxs );

    Vector dir = GetVel().Normalized();

    CUtlVector<CBaseEntity*> vHitEnts;
    bool bHit = MeleeAttackTrace( mins, maxs, GetClawAttackRange(), flDamage, DMG_SLASH, &vHitEnts, &dir );

    FOR_EACH_VEC( vHitEnts, i )
    {
        CBasePlayer* pPlayer = ToBasePlayer( vHitEnts[i] );
        if ( pPlayer )
        {
            pPlayer->ViewPunch( angPunch );
            pPlayer->VelocityPunch( vecPunchVel );
        }
    }


    ClawImpactSound( bHit );

    return bHit;
}

bool CZMBanshee::HasConditionsForRangeAttack( CBaseEntity* pEnemy ) const
{
    if ( gpGlobals->curtime < GetNextLeapAttack() )
        return false;

    if ( GetNextMove() > gpGlobals->curtime )
        return false;


    const Vector vecStart = WorldSpaceCenter();
    const Vector vecEnd = pEnemy->WorldSpaceCenter();

    // Close enough?
    float minleapsqr = zm_sv_banshee_leapdist_min.GetFloat();
    minleapsqr *= minleapsqr;
    float maxleapsqr = zm_sv_banshee_leapdist_max.GetFloat();
    maxleapsqr *= maxleapsqr;
    float distEnemy = vecStart.DistToSqr( vecEnd );
    if ( distEnemy < minleapsqr || distEnemy > maxleapsqr )
    {
        return false;
    }


    bool bDebugging = zm_sv_debug_bansheerangeattack.GetBool();

    // We need to be able to see em.
    trace_t tr;
    UTIL_TraceLine( vecStart, vecEnd, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

    bool bCanSee = tr.fraction == 1.0f || (tr.m_pEnt && tr.m_pEnt->IsPlayer());

    if ( bDebugging )
    {
        NDebugOverlay::Line( vecStart, vecEnd, (!bCanSee) ? 255 : 0, bCanSee ? 255 : 0, 0, true, 0.1f );
    }

    if ( bCanSee )
    {
        // Do a hull trace so it makes sense for us to leap towards the enemy.
        Vector mins, maxs;
        maxs.x = maxs.y = GetMotor()->GetHullWidth() / 2.0f;
        mins.x = mins.y = -maxs.x;
        mins.z = 0.0f;
        maxs.z = GetMotor()->GetHullHeight() - 4.0f;

        Vector vecBoxStart = GetAbsOrigin();
        vecBoxStart.z += 4.0f;

        Vector dir = vecEnd - vecStart;
        float dist = dir.NormalizeInPlace() * 0.5f;
        dist = MAX( 16.0f, dist );

        Vector vecBoxEnd = vecBoxStart + dir * dist;

        UTIL_TraceHull( vecBoxStart, vecBoxEnd, mins, maxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

        bool ret = tr.fraction == 1.0f;

        if ( bDebugging )
        {
            NDebugOverlay::SweptBox( vecBoxStart, vecBoxEnd, mins, maxs, vec3_angle, (!ret) ? 255 : 0, ret ? 255 : 0, 0, 0, 0.1f );
        }

        return ret;
    }

    return false;
}

NPCR::CSchedule<CZMBaseZombie>* CZMBanshee::GetRangeAttackSchedule() const
{
    return m_pLeapSched;
}

void CZMBanshee::HandleAnimEvent( animevent_t* pEvent )
{
    //extern int AE_FASTZOMBIE_CLIMB_LEFT;
    //extern int AE_FASTZOMBIE_CLIMB_RIGHT;

    //if ( pEvent->event == AE_FASTZOMBIE_CLIMB_LEFT || pEvent->event == AE_FASTZOMBIE_CLIMB_RIGHT )
    //{
        //if( ++m_iClimbCount % 3 == 0 )
        //{
			//ENVELOPE_CONTROLLER.SoundChangePitch( m_pLayer2, random->RandomFloat( 100, 150 ), 0.0 );
            //ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pLayer2, SOUNDCTRL_CHANGE_VOLUME, envFastZombieVolumeClimb, ARRAYSIZE(envFastZombieVolumeClimb) );
        //}

    //    return;
    //}
    
    if ( pEvent->event == AE_FASTZOMBIE_GALLOP_LEFT )
    {
        EmitSound( "NPC_FastZombie.GallopLeft" );
        return;
    }

    if ( pEvent->event == AE_FASTZOMBIE_GALLOP_RIGHT )
    {
        EmitSound( "NPC_FastZombie.GallopRight" );
        return;
    }
    
    if (pEvent->event == AE_ZOMBIE_ATTACK_RIGHT
    ||  pEvent->event == AE_ZOMBIE_ATTACK_LEFT)
    {
        Vector right;
        AngleVectors( GetLocalAngles(), NULL, &right, NULL );
        right = right * -50;
        QAngle viewpunch( -3, -5, -3 );

        ClawAttack( GetClawAttackRange(), zm_sk_banshee_dmg_claw.GetInt(), viewpunch, right );
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

NPCR::CSchedule<CZMBaseZombie>* CZMBanshee::OverrideCombatSchedule() const
{
    CZMCommandBase* pCommand = GetCommandQueue()->NextCommand();
    if ( pCommand && pCommand->GetCommandType() == COMMAND_CEILINGAMBUSH )
    {
        GetCommandQueue()->RemoveCommand( pCommand );
        return m_pCeilAmbushSched;
    }

    return BaseClass::OverrideCombatSchedule();
}

void CZMBanshee::OnAnimActivityFinished( Activity completedActivity )
{
    if ( completedActivity == ACT_MELEE_ATTACK1 )
    {
        bool bCanAttack = GetEnemy() && HasConditionsForClawAttack( GetEnemy() );
        if ( !bCanAttack )
        {
            SetActivity( ACT_FASTZOMBIE_FRENZY );

            float delay = SequenceDuration();
            SetNextAttack( gpGlobals->curtime + delay );
            SetNextMove( gpGlobals->curtime + delay );


            EmitSound( "NPC_FastZombie.Frenzy" );
        }

    }
    else if ( completedActivity == ACT_FASTZOMBIE_FRENZY )
    {
        bool bCanAttack = GetEnemy() && HasConditionsForClawAttack( GetEnemy() );
        if ( bCanAttack )
        {
            SetActivity( ACT_FASTZOMBIE_BIG_SLASH );

            float delay = SequenceDuration();
            SetNextAttack( gpGlobals->curtime + delay );
            SetNextMove( gpGlobals->curtime + delay );
        }
    }

    BaseClass::OnAnimActivityFinished( completedActivity );
}

#define ALERT_SOUND_NEAR_DIST       512.0f

void CZMBanshee::AlertSound()
{
    // Apparently banshee doesn't use these at all.
    // And these sounds are weird
    /*
    if ( GetEnemy() )
    {
        EmitSound( GetEnemy()->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) < (ALERT_SOUND_NEAR_DIST*ALERT_SOUND_NEAR_DIST) ?
            "NPC_FastZombie.AlertNear" :
            "NPC_FastZombie.AlertFar" );
    }
    */
}

void CZMBanshee::AttackSound()
{
    EmitSound( "NPC_FastZombie.Attack" );
}

void CZMBanshee::DeathSound()
{
    EmitSound( "NPC_FastZombie.Die" );
}

void CZMBanshee::FootstepSound( bool bRightFoot )
{
}

void CZMBanshee::FootscuffSound( bool bRightFoot )
{
}

void CZMBanshee::ClawImpactSound( bool bHit )
{
    EmitSound( bHit ? "NPC_FastZombie.AttackHit" : "NPC_FastZombie.AttackMiss" );
}

void CZMBanshee::LeapAttackSound()
{
    EmitSound( "NPC_FastZombie.LeapAttack" );
}
