#include "cbase.h"
#include "func_break.h"
#include "npcevent.h"
#include "activitylist.h"
#include "eventlist.h"
#include "collisionutils.h"

#include "npcr_manager.h"

#include "zmr_zombiemodelgroups.h"
#include "zmr_gamerules.h"
#include "zmr_blockerfinder.h"
#include "npcs/zmr_zombieanimstate.h"
#include "npcs/zmr_zombie_senses.h"
#include "zmr_zombiebase.h"
#include "zmr_softcollisions.h"
#include "npcs/zmr_zombiebase_shared.h"
#include "sched/zmr_zombie_main.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_debug_zombie_flinch( "zm_sv_debug_zombie_flinch", "0", 0, "Play flinching animations every time a zombie g ets shot." );


class CZMLOSFilter : public CTraceFilter
{
public:
    virtual bool ShouldHitEntity( IHandleEntity* pEntity, int contentsMask ) OVERRIDE
    {
        CBaseEntity* pEnt = EntityFromEntityHandle( pEntity );
        if ( !pEnt )
            return false;
        if ( pEnt->MyCombatCharacterPointer() != nullptr )
            return false;

        return true;
    }
};

// Passes through everything and will test LOS whether the attack will be successful.
class CZMMeleeFilter : public CTraceFilterEntitiesOnly
{
public:
    CZMMeleeFilter( CZMBaseZombie* pMe, int contentsMask, const CTakeDamageInfo& info )
    {
        Assert( pMe );
        m_pAttacker = pMe;
        m_pDmgInfo = &info;
        m_vecCenter = m_pAttacker->WorldSpaceCenter();
        m_vecAttackPos = m_pAttacker->GetAttackPos();
    }

    bool DidHit() const { return m_vHitEnts.Count() > 0; }
    void CopyHitEntities( CUtlVector<CBaseEntity*>& vec ) const { vec.CopyArray( m_vHitEnts.Base(), m_vHitEnts.Count() ); }

    virtual bool ShouldHitEntity( IHandleEntity* pEntity, int contentsMask ) OVERRIDE
    {
        CBaseEntity* pEnt = EntityFromEntityHandle( pEntity );
        if ( !pEnt )
            return false;
        if ( pEnt == m_pAttacker )
            return false;
        if ( pEnt->IsWorld() )
            return false;
        if ( !pEnt->ShouldCollide( COLLISION_GROUP_PROJECTILE, contentsMask ) )
            return false;
        if ( !g_pGameRules->ShouldCollide( COLLISION_GROUP_PROJECTILE, pEnt->GetCollisionGroup() ) )
            return false;


        //
        // Trace to see if we can see this entity.
        //
        auto CantSee = [ pEnt ]( const trace_t& tr )
        {
            return tr.fraction != 1.0f && tr.m_pEnt != pEnt;
        };
        
        trace_t tr;
        UTIL_TraceLine( m_vecAttackPos, pEnt->WorldSpaceCenter(), MASK_NPCSOLID, &m_Filter, &tr );
        if ( CantSee( tr ) )
        {
            // We didn't hit, check their eye position as well
            // Fixes some edge cases where the body is hidden even though
            // the eyes are clearly visible.
            if ( pEnt->MyCombatCharacterPointer() != nullptr )
            {
                UTIL_TraceLine( m_vecAttackPos, pEnt->EyePosition(), MASK_NPCSOLID, &m_Filter, &tr );
                if ( CantSee( tr ) )
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }



        bool bDamage = false;

        if ( pEnt->MyCombatCharacterPointer() != nullptr )
        {
            // Don't damage allies, silly.
            bDamage = m_pAttacker->IsEnemy( pEnt );
        }
        else if ( m_pAttacker->CanBreakObject( pEnt ) )
        {
            bDamage = true;

            Pickup_ForcePlayerToDropThisObject( pEnt );
        }

        if ( bDamage )
        {
            CTakeDamageInfo info = *m_pDmgInfo;

            Vector dir = pEnt->WorldSpaceCenter() - m_vecCenter;
            dir.NormalizeInPlace();

            CalculateMeleeDamageForce( &info, dir, tr.endpos );

            pEnt->TakeDamage( info );


            m_vHitEnts.AddToTail( pEnt );
        }

        return false;
    }

private:
    CZMBaseZombie*              m_pAttacker;
    int                         m_fMask;
    const CTakeDamageInfo*      m_pDmgInfo;
    Vector                      m_vecCenter;
    Vector                      m_vecAttackPos;
    CZMLOSFilter                m_Filter;

    CUtlVector<CBaseEntity*>    m_vHitEnts;
};



int CZMBaseZombie::AE_ZOMBIE_ATTACK_RIGHT = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_ATTACK_LEFT = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_ATTACK_BOTH = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_SWATITEM = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_GET_UP = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_POUND = AE_INVALID;
int CZMBaseZombie::AE_ZOMBIE_ALERTSOUND = AE_INVALID;

float CZMBaseZombie::g_flLastZombieSound = 0.0f;



ConVar zm_sv_swatmaxmass( "zm_sv_swatmaxmass", "200", FCVAR_NOTIFY );
ConVar zm_sv_swatlift( "zm_sv_swatlift", "20000", FCVAR_NOTIFY, "How much upward force is applied to the swatted prop." );
ConVar zm_sv_swatforcemin( "zm_sv_swatforcemin", "20000", FCVAR_NOTIFY, "Minimum amount of force applied to the swatted prop." );
ConVar zm_sv_swatforcemax( "zm_sv_swatforcemax", "50000", FCVAR_NOTIFY, "Maximum amount of force applied to the swatted prop." ); // Originally 70000
ConVar zm_sv_swatangvel( "zm_sv_swatangvel", "1000", FCVAR_NOTIFY, "Amount of angular velocity swatting applies to prop." );


ConVar zm_sv_swat_scan_target_maxdist( "zm_sv_swat_scan_target_maxdist", "512", FCVAR_NOTIFY, "How far our target (enemy) can be for us to swat objects." );
ConVar zm_sv_swat_scan_def_maxdist( "zm_sv_swat_scan_def_maxdist", "128", FCVAR_NOTIFY, "If no enemy, how far do we look for objects." );

ConVar zm_sv_defense_chase_dist( "zm_sv_defense_chase_dist", "333", FCVAR_NOTIFY, "Zombies will begin chasing if enemy is closer than this." );
ConVar zm_sv_defense_goal_tolerance( "zm_sv_defense_goal_tolerance", "64", FCVAR_NOTIFY, "How close is 'home' for a zombie going back to the spot being defended." );


ConVar zm_sv_debug_zombieattack( "zm_sv_debug_zombieattack", "0" );
ConVar zm_sv_debug_shotgun_dmgmult( "zm_sv_debug_shotgun_dmgmult", "0" );

ConVar zm_sv_zombiesoftcollisions( "zm_sv_zombiesoftcollisions", "1", FCVAR_NOTIFY, "Toggle experimental zombie collisions." );

ConVar zm_sv_zombie_stepheight( "zm_sv_zombie_stepheight", "17", FCVAR_NOTIFY, "The default zombie step height." );


extern ConVar zm_sk_default_hitmult_head;
extern ConVar zm_sk_default_hitmult_head_buckshot;
extern ConVar zm_sk_default_hitmult_head_buckshot_dist;




IMPLEMENT_SERVERCLASS_ST( CZMBaseZombie, DT_ZM_BaseZombie )
    // Send low-resolution
    SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
    SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

    // Only send yaw.
    SendPropAngle( SENDINFO_VECTORELEM( m_angRotation, 1 ), 10, SPROP_CHANGES_OFTEN ),
    SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

    // Don't need these
    SendPropExclude( "DT_BCCLocalPlayerExclusive", "m_flNextAttack" ),
    SendPropExclude( "DT_BaseCombatCharacter", "m_hActiveWeapon" ),
    SendPropExclude( "DT_BaseCombatCharacter", "m_hMyWeapons" ),


    SendPropInt( SENDINFO( m_iSelectorIndex ) ),
    SendPropFloat( SENDINFO( m_flHealthRatio ) ),
    SendPropBool( SENDINFO( m_bIsOnGround ) ),
    SendPropInt( SENDINFO( m_iAnimationRandomSeed ) ),
    SendPropInt( SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),

    SendPropInt( SENDINFO( m_iGibType ), Q_log2( ZMGIBTYPE_MAX ) + 1, SPROP_UNSIGNED ),


    // Animation excludes
    SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
    SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
    SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    // Animstate and clientside animation takes care of these on the client
    SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

    SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
    SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
    SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMBaseZombie )
    DEFINE_KEYFIELD( m_strModelGroup, FIELD_STRING, "modelgroup" ),
END_DATADESC()


CZMBaseZombieMotor::CZMBaseZombieMotor( CZMBaseZombie* pOuter ) : NPCR::CNonPlayerMotor( pOuter )
{
    
}

float CZMBaseZombieMotor::GetStepHeight() const
{
    return zm_sv_zombie_stepheight.GetFloat();
}

// ZMRTODO: Balance correctly.
float CZMBaseZombieMotor::GetYawRate( float delta ) const
{
    return RemapValClamped( fabsf( delta ), 0.0f, 60.0f, 20.0f, 200.0f );
}


CZMBaseZombie::CZMBaseZombie()
{
    UseClientSideAnimation();



    m_hSwatObject.Set( nullptr );

    m_flLastCommanded = 0.0f;

    m_iZombieMode = ZOMBIEMODE_OFFENSIVE;


    m_iSelectorIndex = 0;
    m_flHealthRatio = 1.0f;
    m_bIsOnGround = false;
    static int randomseed = 0;
    randomseed = (randomseed + 1) % 50;
    m_iAnimationRandomSeed = (int)gpGlobals->curtime + randomseed;
    
    m_iAdditionalAnimRandomSeed = 0;

    m_iGibType = ZMGIBTYPE_NONE;


    m_hAmbushEnt.Set( nullptr );


    m_flNextMove = 0.0f;


    m_flBurnDamage = 0.0f;
    m_flBurnDamageTime = 0.0f;

    m_flNextIdleSound = 0.0f;
    m_flNextPainSound = 0.0f;


    m_strModelGroup = NULL_STRING;

    ChangeTeam( ZMTEAM_ZM );


    g_ZombieManager.AddZombie( this );

    m_MyCommandStyle = ZCOMMANDSTYLE_DEFAULT;
}

CZMBaseZombie::~CZMBaseZombie()
{
    delete m_pAnimState;
    delete m_pBlockerScanner;


    RemoveFromAmbush( true );

    g_ZombieManager.RemoveZombie( this );



    // It's safe to remove pop count here.
    CZMRules* pRules = ZMRules();
    if ( pRules )
    {
        pRules->SetZombiePop( pRules->GetZombiePop() - GetPopCost() );
    }
}

bool CZMBaseZombie::CreateComponents()
{
    bool res = BaseClass::CreateComponents();
    if ( !res )
        return false;

    m_pAnimState = CreateAnimState();
    if ( !m_pAnimState )
        return false;


    m_pBlockerScanner = CreateBlockerScanner();
    

    return true;
}

NPCR::CScheduleInterface* CZMBaseZombie::CreateScheduleInterface()
{
    return new NPCR::CScheduleInterface( this, new CZombieMainSchedule );
}

NPCR::CBaseSenses* CZMBaseZombie::CreateSenses()
{
    return new CZMZombieSenses( this );
}

NPCR::CNonPlayerMotor* CZMBaseZombie::CreateMotor()
{
    return new CZMBaseZombieMotor( this );
}

CZMZombieAnimState* CZMBaseZombie::CreateAnimState()
{
    return new CZMZombieAnimState( this );
}

CZMBlockerScanner* CZMBaseZombie::CreateBlockerScanner()
{
    return new CZMBlockerScanner( this );
}

void CZMBaseZombie::Precache()
{
    BaseClass::Precache();

    PrecacheScriptSound( "Zombie.AttackHit" );
    PrecacheScriptSound( "Zombie.AttackMiss" );

    PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
    PrecacheScriptSound( "NPC_BaseZombie.Swat" );


    PrecacheScriptSound( "BaseCombatCharacter.CorpseGib" );
    PrecacheScriptSound( "NPC_Antlion.RunOverByVehicle" );


    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_SWATITEM );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_GET_UP );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_POUND );
    REGISTER_PRIVATE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND );
}

void CZMBaseZombie::Spawn()
{
    g_ZombieModelGroups.OnZombieSpawn( this );


    BaseClass::Spawn();


    SetThink( &CZMBaseZombie::ZombieThink );
    SetNextThink( gpGlobals->curtime );


    AddSolidFlags( FSOLID_NOT_STANDABLE );
}

bool CZMBaseZombie::IsEnemy( CBaseEntity* pEnt ) const
{
    if ( !pEnt )
        return false;

#ifdef _DEBUG
    if ( pEnt->GetFlags() & FL_NOTARGET )
        return false;
#endif

    return pEnt->IsPlayer() && pEnt->GetTeamNumber() == ZMTEAM_HUMAN && pEnt->IsAlive();
}

NPCR::CPathCostGroundOnly* CZMBaseZombie::GetPathCost() const
{
    static NPCR::CPathCostGroundOnly cost;
    return &cost;
}

void CZMBaseZombie::ZombieThink()
{
    VPROF_BUDGET( "CZMBaseZombie::ZombieThink", "NPCR" );


    m_pAnimState->Update();

    BaseClass::NPCThink();
}

void CZMBaseZombie::PreUpdate()
{
    BaseClass::PreUpdate();


    if ( ShouldPlayIdleSound() )
    {
        float delay = IdleSound();

        m_flNextIdleSound = gpGlobals->curtime + delay;
    }
}

void CZMBaseZombie::PostUpdate()
{
    BaseClass::PostUpdate();


    m_bIsOnGround = GetMotor()->IsOnGround();


    m_CmdQueue.UpdateExpire();
}

void CZMBaseZombie::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_ZOMBIE_POUND )
    {
        //PoundSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
    {
        AlertSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_GET_UP )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STARTSWAT )
    {
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
    {
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

bool CZMBaseZombie::Event_Gibbed( const CTakeDamageInfo &info )
{
    CorpseGib( info );

    return true;
}

bool CZMBaseZombie::CorpseGib( const CTakeDamageInfo &info )
{
    // ZMRTODO: Combine these into one single sound.
    EmitSound( "BaseCombatCharacter.CorpseGib" );
    EmitSound( "NPC_Antlion.RunOverByVehicle" );


    // ZMRTODO: Make sure CanBecomeRagdoll always return true!!!
    if ( !BecomeRagdollOnClient( vec3_origin ) )
    {
        UTIL_Remove( this );
    }
    else
    {
        m_iGibType = ZMGIBTYPE_HEAD;
    }

    return false;
}

bool CZMBaseZombie::ShouldGib( const CTakeDamageInfo& info )
{
    if ( info.GetDamageType() & DMG_ALWAYSGIB ) return true;


    return BaseClass::ShouldGib( info );
}

void CZMBaseZombie::Extinguish()
{
    DoAnimationEvent( ZOMBIEANIMEVENT_ON_EXTINGUISH );

    BaseClass::Extinguish();
}

void CZMBaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
    // We are an NPC! :/
    bNPCOnly = false;


    if ( !IsOnFire() && IsAlive() )
    {
        // Tell our animstate to play burning idle & walking animations.
        DoAnimationEvent( ZOMBIEANIMEVENT_ON_BURN );
    }

    BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );
}

bool CZMBaseZombie::IsAttacking() const
{
    Activity act = GetActivity();

    if ( act == ACT_MELEE_ATTACK1 )
        return true;

    return false;
}

bool CZMBaseZombie::CanMove() const
{
    if ( GetNextMove() > gpGlobals->curtime )
        return false;

    return true;
}

void CZMBaseZombie::GetAttackHull( Vector& mins, Vector& maxs ) const
{
    mins = CollisionProp()->OBBMins();
    maxs = CollisionProp()->OBBMaxs();

    mins.z = GetAttackLowest();
    maxs.z = GetAttackHeight();
}

const Vector CZMBaseZombie::GetAttackPos() const
{
    Vector pos = GetAbsOrigin();
    pos.z += CollisionProp()->OBBMaxs().z * 0.7f; // About shoulder height.

    return pos;
}

float CZMBaseZombie::GetAttackHeight() const
{
    return CollisionProp()->OBBMaxs().z + 20.0f;
}

float CZMBaseZombie::GetAttackLowest() const
{
    return 8.0f;
}

bool CZMBaseZombie::HasConditionsForClawAttack( CBaseEntity* pEnemy ) const
{
    // Not close enough in XY?
    float dist = GetClawAttackRange();
    if ( pEnemy->GetAbsOrigin().AsVector2D().DistToSqr( GetAbsOrigin().AsVector2D() ) > (dist*dist) )
        return false;

    float up_z = GetAbsOrigin().z + GetAttackHeight();
    if ( pEnemy->GetAbsOrigin().z > up_z )
        return false;

    // Take into account the collision model.
    float down_z = GetAbsOrigin().z + GetAttackLowest() - pEnemy->CollisionProp()->OBBMaxs().z;
    if ( pEnemy->GetAbsOrigin().z < down_z )
        return false;


    return true;
}

float CZMBaseZombie::GetClawAttackRange() const
{
    return 55;
}

CBaseEntity* CZMBaseZombie::ClawAttack( float flDist, float flDamage, const QAngle& angPunch, const Vector& vecPunchVel )
{
    Vector vecMins, vecMaxs;
    GetAttackHull( vecMins, vecMaxs );
    Assert( vecMaxs.z > vecMins.z );


    CUtlVector<CBaseEntity*> vHitEnts;
    bool bHit = MeleeAttackTrace( vecMins, vecMaxs, flDist, flDamage, DMG_SLASH, &vHitEnts );

    for ( int i = 0; i < vHitEnts.Count(); i++ )
    {
        CBasePlayer* pHurtPlayer = ToBasePlayer( vHitEnts[i] );
        if ( pHurtPlayer )
        {
            pHurtPlayer->ViewPunch( angPunch );
            pHurtPlayer->VelocityPunch( vecPunchVel );
        }
        else if ( CanBreakObject( vHitEnts[i] ) )
        {
            SwatObject( vHitEnts[i] );
        }
    }

    /*if ( !pHurt && GetSwatObject() && IsCurSchedule( SCHED_ZOMBIE_ATTACKITEM ) )
    {
        pHurt = GetSwatObject();

        Vector vForce = pHurt->WorldSpaceCenter() - WorldSpaceCenter(); 
        VectorNormalize( vForce );

        vForce *= 5 * 24;

        CTakeDamageInfo info( this, this, vForce, GetAbsOrigin(), iDamage, DMG_SLASH );
        pHurt->TakeDamage( info );
    }*/

    /*if ( pHurt == m_hPhysicsEnt.Get() && IsCurSchedule( SCHED_ZOMBIE_ATTACKITEM ) )
    {
        m_hPhysicsEnt.Set( nullptr );
        m_flNextSwat = gpGlobals->curtime + random->RandomFloat( 2, 4 );
    }*/

    ClawImpactSound( bHit );


    // Tell our components we just attacked.
    OnAttacked();


    return nullptr;
}

bool CZMBaseZombie::MeleeAttackTrace(
    const Vector& vecMins, const Vector& vecMaxs,
    float flDist,
    float flDamage, int iDmgType,
    CUtlVector<CBaseEntity*>* vHitEnts,
    const Vector* vecDir )
{
    const Vector vecStart = GetAbsOrigin();
    
    Vector fwd;
    if ( vecDir )
    {
        fwd = *vecDir;
        fwd.NormalizeInPlace();
    }
    else
    {
        QAngle ang = GetAbsAngles();
        AngleVectors( ang, &fwd );
    }

    const Vector vecEnd = vecStart + fwd * flDist;


    CTakeDamageInfo	info( this, this, flDamage, iDmgType );

    CZMMeleeFilter filter( this, MASK_SHOT_HULL, info );

    trace_t tr;
    UTIL_TraceHull( vecStart, vecEnd, vecMins, vecMaxs, MASK_SHOT_HULL, &filter, &tr );


    bool bDidHit = filter.DidHit();

    if ( zm_sv_debug_zombieattack.GetBool() )
    {
        NDebugOverlay::SweptBox( vecStart, vecEnd, vecMins, vecMaxs, vec3_angle, bDidHit ? 255 : 0, (!bDidHit) ? 255 : 0, 0, 0, 1.0f );
    }

    if ( vHitEnts )
    {
        filter.CopyHitEntities( *vHitEnts );
    }

    return bDidHit;
}

void CZMBaseZombie::ClawImpactSound( bool bHit )
{
    EmitSound( bHit ? "Zombie.AttackHit" : "Zombie.AttackMiss" );
}

float CZMBaseZombie::GetSwatMaxMass()
{
    return zm_sv_swatmaxmass.GetFloat();
}

bool CZMBaseZombie::CanSwatObject( CBaseEntity* pEnt )
{
    if ( !pEnt )
        return false;
    
    if ( pEnt->MyCombatCharacterPointer() != nullptr )
        return false;

    // Ignore projectiles.
    if ( pEnt->IsCombatItem() )
        return false;

    // Unfortunately need this exception because func_brush has vphysics and always moveable even though it can't really be moved.
    // Example: zm_asylum elevator floor
    if ( FClassnameIs( pEnt, "func_brush" ) )
        return false;
    //if ( FClassnameIs( pEnt, "func_breakable" ) )
    //    return false;
    //if ( FClassnameIs( pEnt, "func_breakable_surf" ) )
    //    return false;


    // If we're a physics object that moves, sure.
    IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
    
    if ( !pPhys )
        return false;


    return pPhys->IsMoveable() && pPhys->GetMass() < GetSwatMaxMass();
}

bool CZMBaseZombie::CanBreakObject( CBaseEntity* pEnt, bool bSwat ) const
{
    if ( !pEnt )
        return false;

    if ( pEnt->GetHealth() <= 0 || pEnt->m_takedamage != DAMAGE_YES )
        return false;

    // Avoid breaking our own stuff.
    if ( pEnt->GetTeamNumber() == ZMTEAM_ZM )
        return false;

    CBreakable* pBreak = dynamic_cast<CBreakable*>( pEnt );
    if ( pBreak && pBreak->IsBreakable() )
        return true;

    IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
    if ( pPhys )
    {
        if ( !bSwat && !pPhys->IsMotionEnabled() )
            return true;

        // Swat that shit away from their hands.
        if ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
            return true;
    }
        

    return false;
}

#define ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST	96.0f // Triple damage from buckshot at 8 feet (headshot only)

void CZMBaseZombie::TraceAttack( const CTakeDamageInfo& inputInfo, const Vector& vecDir, trace_t* pTrace, CDmgAccumulator* pAccumulator )
{
    // The force bone will be used for proper ragdolling.
    m_nForceBone = pTrace->physicsbone;


    if ( zm_sv_debug_zombie_flinch.GetBool() )
    {
        if ( pTrace->hitgroup == HITGROUP_HEAD )
        {
            DoAnimationEvent( ZOMBIEANIMEVENT_GESTURE, ACT_GESTURE_FLINCH_CHEST );
        }
        else if ( pTrace->hitgroup == HITGROUP_RIGHTARM )
        {
            DoAnimationEvent( ZOMBIEANIMEVENT_GESTURE, ACT_GESTURE_FLINCH_RIGHTARM );
        }
        else if ( pTrace->hitgroup == HITGROUP_LEFTARM )
        {
            DoAnimationEvent( ZOMBIEANIMEVENT_GESTURE, ACT_GESTURE_FLINCH_LEFTARM );
        }
    }



    CTakeDamageInfo info = inputInfo;

    ScaleDamageByHitgroup( pTrace->hitgroup, info );



    BaseClass::TraceAttack( info, vecDir, pTrace, pAccumulator );
}

bool CZMBaseZombie::ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const
{
    // Don't scale explosive damage.
    if ( info.GetDamageType() & DMG_BLAST )
        return true;


    switch ( iHitGroup )
    {
    case HITGROUP_HEAD :
        {
            // Shotgun scales damage differently.
            if ( info.GetDamageType() & DMG_BUCKSHOT )
            {
                MultiplyBuckshotDamage( info );
            }
            // ZMRTODO: This random shit really has to go.
            else if ( info.GetDamageType() & DMG_CLUB )
            {
                info.ScaleDamage( random->RandomFloat( 1.9f, 3.0f ) );
            }
            else
            {
                info.ScaleDamage( zm_sk_default_hitmult_head.GetFloat() );
            }

            return true;
        }
    }

    return false;
}

void CZMBaseZombie::MultiplyBuckshotDamage( CTakeDamageInfo& info ) const
{
    CBaseEntity* pAttacker = info.GetAttacker();
    if ( !pAttacker ) return;


    const Vector startpos = pAttacker->EyePosition();
    const Vector hitpos = info.GetDamagePosition();


    float flDistSqr = startpos.DistToSqr( hitpos );

    float flDmgDistSqr = zm_sk_default_hitmult_head_buckshot_dist.GetFloat();
    flDmgDistSqr *= flDmgDistSqr;

    // We need to be close enough to do more damage.
    const bool bScaleDmg = flDistSqr < flDmgDistSqr;


    if ( zm_sv_debug_shotgun_dmgmult.GetBool() )
    {
        float t = zm_sv_debug_shotgun_dmgmult.GetFloat();
        NDebugOverlay::Axis( hitpos, vec3_angle, 2.0f, true, t );
        NDebugOverlay::Line( startpos, hitpos, (!bScaleDmg) ? 255 : 0, bScaleDmg ? 255 : 0, 0, true, t );
    }


    if ( bScaleDmg )
    {
        info.ScaleDamage( zm_sk_default_hitmult_head_buckshot.GetFloat() );
    }
}

void CZMBaseZombie::Event_Killed( const CTakeDamageInfo& info )
{
    StopLoopingSounds();
    DeathSound();

    BaseClass::Event_Killed( info );
}

#define ZOMBIE_BULLET_DAMAGE_SCALE  0.5f

#define ZOMBIE_SCORCH_RATE          8
#define ZOMBIE_MIN_RENDERCOLOR      50
int CZMBaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo& inputInfo )
{
    CTakeDamageInfo info = inputInfo;

    
    if ( info.GetDamageType() & DMG_BURN )
    {
        // If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
        // This is to stop zombies from burning to death 10x faster when they're standing around
        // 10 fire entities.
        if( IsOnFire() && !(info.GetDamageType() & DMG_DIRECT) )
        {
            return 0;
        }
        
        //Scorch( ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR );
    }
    

    if ( ShouldIgnite( info ) )
    {
        Ignite( 100.0f );
    }

    // Take some percentage of damage from bullets (unless hit in the crab). Always take full buckshot & sniper damage
    if ( info.GetDamageType() & DMG_BULLET && !(info.GetDamageType() & (DMG_BUCKSHOT/*|DMG_SNIPER*/)) )
    {
        info.ScaleDamage( ZOMBIE_BULLET_DAMAGE_SCALE );
    }

    int tookDamage = BaseClass::OnTakeDamage_Alive( info );

    if ( tookDamage )
    {
        m_flHealthRatio = m_iHealth / (float)(m_iMaxHealth > 0 ? m_iMaxHealth : 1);
    }


    if ( ShouldPlayPainSound( info ) )
    {
        float delay = PainSound( info );

        m_flNextPainSound = gpGlobals->curtime + delay;
    }


    return tookDamage;
}

bool CZMBaseZombie::ShouldIgnite( const CTakeDamageInfo& info )
{
    if ( IsOnFire() )
    {
        // Already burning!
        return false;
    }

    if ( info.GetDamageType() & DMG_BURN )
    {
        //
        // If we take more than ten percent of our health in burn damage within a five
        // second interval, we should catch on fire.
        //
        if ( (m_flBurnDamageTime + 5.0f) < gpGlobals->curtime )
        {
            m_flBurnDamage = 0.0f;
        }

        m_flBurnDamage += info.GetDamage();
        m_flBurnDamageTime = gpGlobals->curtime;

        if ( m_flBurnDamage >= (m_iMaxHealth * 0.1f) )
        {
            return true;
        }
    }

    return false;
}

bool CZMBaseZombie::Swat( CZMPlayer* pZM, CBaseEntity* pSwat, bool bBreak )
{
    if ( !pSwat ) return false;

    if ( !CanSwatPhysicsObjects() && !bBreak ) return false;


    m_hSwatObject.Set( pSwat );

    //m_bSwatBreakable = bBreak;


    m_CmdQueue.QueueCommand( new CZMCommandSwat( pZM, pSwat, bBreak ) );
    OnQueuedCommand( pZM, COMMAND_SWAT );

    SetMyCommandStyle( pZM );

    return true;
}

bool CZMBaseZombie::SwatObject( CBaseEntity* pSwat )
{
    if ( !pSwat )
        return false;


    IPhysicsObject* pPhys = pSwat->VPhysicsGetObject();
    if ( !pPhys )
        return false;


    Vector dir;
    CBaseEntity* pEnemy = GetEnemy();
    if ( pEnemy )
    {
        PhysicsImpactSound( pEnemy, pPhys, CHAN_BODY, pPhys->GetMaterialIndex(), physprops->GetSurfaceIndex( "flesh" ), 0.5, 800 );

        dir = pEnemy->WorldSpaceCenter() - pSwat->WorldSpaceCenter();
        dir.NormalizeInPlace();
    }
    else
    {
        AngleVectors( QAngle( 0.0f, EyeAngles().y, 0.0f ), &dir );
    }


    int mass = pPhys->GetMass();

    Vector uplift = Vector( 0, 0, RemapVal( mass, 5, 350, 3000, zm_sv_swatlift.GetFloat() ) );

    float force = RemapVal( mass, 5, 500, zm_sv_swatforcemin.GetFloat(), zm_sv_swatforcemax.GetFloat() );


    //if ( pPhys->IsAsleep() && pPhys->IsMotionEnabled() )
    //{
    //    pPhys->Wake();
    //}


    // Force the player to drop it!
    if ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
    {
        auto* pPlayer = ToZMPlayer( pSwat->GetOwnerEntity() );
        
        if ( pPlayer )
        {
            pPlayer->ForceDropOfCarriedPhysObjects( pSwat );
        }
    }

    pPhys->ApplyForceCenter( dir * force + uplift );


            
    // Add a bit of spin so it doesn't look boring.
    float f = zm_sv_swatangvel.GetFloat();

    if ( f > 0.0f )
    {
        AngularImpulse angvel(
            random->RandomFloat( -f, f ),
            random->RandomFloat( -f, f ),
            random->RandomFloat( -f, f ) );

        pPhys->AddVelocity( nullptr, &angvel );
    }


    return true;
}

void CZMBaseZombie::Command( CZMPlayer* pZM, const Vector& vecPos, float flTolerance )
{
    SetMyCommandStyle( pZM );

    m_CmdQueue.QueueCommand( new CZMCommandMove( pZM, vecPos ) );
    OnQueuedCommand( pZM, COMMAND_MOVE );
}

//
// Let our commander dictate certain behaviors.
//
void CZMBaseZombie::SetMyCommandStyle( const CZMPlayer* pZM )
{
    if ( pZM )
    {
        m_MyCommandStyle = pZM->GetZMCommandStyle();
    }
}

bool CZMBaseZombie::CanSpawn( const Vector& vecPos ) const
{
    trace_t trace;
    Vector up = vecPos + Vector( 0.0f, 0.0f, 1.0f );

    Vector mins, maxs;
    maxs.x = GetMotor()->GetHullWidth() / 2.0f;
    maxs.y = maxs.x;
    maxs.z = GetMotor()->GetHullHeight();

    mins.x = mins.y = -maxs.x;
    mins.z = 0.0f;


    UTIL_TraceHull( vecPos, up, mins, maxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &trace );


    return trace.fraction == 1.0f;
}


void CZMBaseZombie::SetAmbush( CZMEntAmbushTrigger* pTrigger )
{
    m_hAmbushEnt.Set( pTrigger );
    SetZombieMode( ZOMBIEMODE_AMBUSH );
}

void CZMBaseZombie::RemoveFromAmbush( bool bRemoveFromAmbushEnt )
{
    if ( bRemoveFromAmbushEnt && m_hAmbushEnt.Get() )
    {
        m_hAmbushEnt.Get()->RemoveZombieFromAmbush();
    }

    m_hAmbushEnt.Set( nullptr );


    SetZombieMode( ZOMBIEMODE_OFFENSIVE );
}

bool CZMBaseZombie::ShouldUpdate() const
{
    if ( m_lifeState == LIFE_DEAD )
        return false;

    return BaseClass::ShouldUpdate();
}

NPCR::CFollowNavPath* CZMBaseZombie::GetFollowPath() const
{
    return new NPCR::CFollowNavPath;
}

NPCR::QueryResult_t CZMBaseZombie::ShouldChase( CBaseEntity* pEnemy ) const
{
    if ( (gpGlobals->curtime - GetLastTimeCommanded()) < 1.0f )
        return NPCR::RES_NO;

    return BaseClass::ShouldChase( pEnemy );
}

NPCR::QueryResult_t CZMBaseZombie::ShouldTouch( CBaseEntity* pEnt ) const
{
    // Fixes a few PARTICULAR maps that use this old terrible method of letting zombies pass through brushes.
    // Here you go, Psycho.
    if ( pEnt->IsBSPModel() )
    {
        auto* pFuncBrush = dynamic_cast<CFuncBrush*>( pEnt );


        // Should we bother doing classname and name matching at all?
        bool bShouldCheck = pFuncBrush ? (pFuncBrush->m_iszExcludedClass != NULL_STRING || pFuncBrush->m_bInvertExclusion) : false;


        if ( pFuncBrush && bShouldCheck )
        {
            auto* pMe = const_cast<CZMBaseZombie*>( this );
            

            bool bMatches;

            bool bClassMatch = pMe->ClassMatches( pFuncBrush->m_iszExcludedClass );
            // Have a name? Try comparing that also.
            bool bNameMatch = STRING( pMe->GetEntityName() )[0] != NULL && pMe->NameMatches( pFuncBrush->m_iszExcludedClass );
            

            bMatches = bClassMatch || bNameMatch;

            if ( pFuncBrush->m_bInvertExclusion )
            {
                bMatches = !bMatches;
            }

            return bMatches ? NPCR::RES_NO : NPCR::RES_YES;
        }
    }
    else if ( pEnt->IsBaseZombie() )
    {
        NPCR::CBaseNPC* pOther = pEnt->MyNPCRPointer();


        if ( zm_sv_zombiesoftcollisions.GetBool() )
        {
            GetZMSoftCollisions()->OnZombieCollide(
                const_cast<CZMBaseZombie*>( this ),
                static_cast<CZMBaseZombie*>( pEnt ) );

            return NPCR::RES_NO;
        }


        // If we're inside each other, don't collide
        const Vector vecMyPos = GetAbsOrigin();
        const Vector vecEntPos = pEnt->GetAbsOrigin();

        Vector mymins, mymaxs, otmins, otmaxs;
        mymaxs.x = mymaxs.y = GetMotor()->GetHullWidth() / 2.0f;
        mymaxs.z = GetMotor()->GetHullHeight();
        mymins.x = mymins.y = -mymaxs.x;
        mymins.z = 0.0f;


        otmaxs.x = otmaxs.y = pOther->GetMotor()->GetHullWidth() / 2.0f;
        otmaxs.z = pOther->GetMotor()->GetHullHeight();
        otmins.x = otmins.y = -otmaxs.x;
        otmins.z = 0.0f;

        mymins += vecMyPos;
        mymaxs += vecMyPos;
        
        otmins += vecEntPos;
        otmaxs += vecEntPos;

        
        if ( IsBoxIntersectingBox( mymins, mymaxs, otmins, otmaxs ) )
            return NPCR::RES_NO;
    }

    return BaseClass::ShouldTouch( pEnt );
}

float CZMBaseZombie::GetMoveActivityMovementSpeed()
{
    return GetAnimState()->GetMaxGroundSpeed();
}

bool CZMBaseZombie::ShouldPlayIdleSound() const
{
    // Don't spam zombie sounds.
    if ( (g_flLastZombieSound + 0.05f) > gpGlobals->curtime )
        return false;


    if ( m_lifeState != LIFE_ALIVE )
        return false;

    if ( HasSpawnFlags( SF_NPC_GAG ) )
        return false;

    if ( m_flNextIdleSound > gpGlobals->curtime )
        return false;


    return true;
}

bool CZMBaseZombie::ShouldPlayPainSound( const CTakeDamageInfo& info ) const
{
    // Don't spam zombie sounds.
    if ( (g_flLastZombieSound + 0.05f) > gpGlobals->curtime )
        return false;

    if ( m_flNextPainSound > gpGlobals->curtime )
        return false;

    if ( m_lifeState != LIFE_ALIVE || m_iHealth <= 0 )
        return false;

    if ( HasSpawnFlags( SF_NPC_GAG ) )
        return false;

    int fDmg = info.GetDamageType();
    if ( fDmg == DMG_GENERIC || fDmg & DMG_ALWAYSGIB )
        return false;

    return true;
}

void CZMBaseZombie::GetAnimRandomSeed( int iEvent, int& nData )
{
    // Only these events should have a random seed in the data section.
    // The rest may have the activity number.
    static int randomseed = 0;
    switch ( iEvent )
    {
    case ZOMBIEANIMEVENT_IDLE :
    case ZOMBIEANIMEVENT_ATTACK :
    case ZOMBIEANIMEVENT_ON_BURN :
    case ZOMBIEANIMEVENT_ON_EXTINGUISH :
        nData = ( randomseed = (randomseed + 1) % 50 );
    default : break;
    }
}
