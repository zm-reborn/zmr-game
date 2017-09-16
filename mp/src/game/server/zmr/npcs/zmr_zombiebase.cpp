#include "cbase.h"
#include "npcevent.h"
#include "gib.h"
#include "func_break.h"
#include "baseanimating.h"
#include "npc_playercompanion.h"
#include "EntityFlame.h"
#include "ai_memory.h"
#include "ai_route.h"

#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"

#include "zmr_zombiebase.h"


#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())


ConVar zombie_basemin( "zombie_basemin", "100" );
ConVar zombie_basemax( "zombie_basemax", "100" );

ConVar zombie_changemin( "zombie_changemin", "0" );
ConVar zombie_changemax( "zombie_changemax", "0" );

// play a sound once in every zombie_stepfreq steps
ConVar zombie_stepfreq( "zombie_stepfreq", "4" );
ConVar zombie_moanfreq( "zombie_moanfreq", "1" );

ConVar zombie_decaymin( "zombie_decaymin", "0.1" );
ConVar zombie_decaymax( "zombie_decaymax", "0.4" );

ConVar zombie_ambushdist( "zombie_ambushdist", "16000" );



#define ZOMBIE_BULLET_DAMAGE_SCALE 0.5f


int AE_ZOMBIE_ATTACK_RIGHT;
int AE_ZOMBIE_ATTACK_LEFT;
int AE_ZOMBIE_ATTACK_BOTH;
int AE_ZOMBIE_SWATITEM;
int AE_ZOMBIE_STARTSWAT;
int AE_ZOMBIE_STEP_LEFT;
int AE_ZOMBIE_STEP_RIGHT;
int AE_ZOMBIE_SCUFF_LEFT;
int AE_ZOMBIE_SCUFF_RIGHT;
int AE_ZOMBIE_ATTACK_SCREAM;
int AE_ZOMBIE_GET_UP;
int AE_ZOMBIE_POUND;
int AE_ZOMBIE_ALERTSOUND;

int CZMBaseZombie::ACT_ZOM_SWATLEFTMID;
int CZMBaseZombie::ACT_ZOM_SWATRIGHTMID;
int CZMBaseZombie::ACT_ZOM_SWATLEFTLOW;
int CZMBaseZombie::ACT_ZOM_SWATRIGHTLOW;
int CZMBaseZombie::ACT_ZOM_RELEASECRAB;
int CZMBaseZombie::ACT_ZOM_FALL;


#define ZOMBIE_FLINCH_DELAY             3.0f

#define ZOMBIE_BURN_TIME                10.0f // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE          2.0f  // Give or take this many seconds.


#define ZOMBIE_PHYSOBJ_SWATDIST         80.0f

#define ZOMBIE_MAX_PHYSOBJ_MASS         200

#define ZOMBIE_PLAYER_MAX_SWAT_DIST     512.0f

#define ZOMBIE_PHYSICS_SEARCH_DEPTH     32
#define ZOMBIE_FARTHEST_PHYSICS_OBJECT  128.0f


// The time zombies will ignore enemies when ZM commands them.
#define COMMANDED_SEE_ENEMY_GRACE       1.0f


#define DEFEND_DIST_CHASENEMY_SQR       ( 333.0f * 333.0f )


IMPLEMENT_SERVERCLASS_ST( CZMBaseZombie, DT_ZM_BaseZombie )
    SendPropInt( SENDINFO( m_iSelectorIndex ) ),
    SendPropFloat( SENDINFO( m_flHealthRatio ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMBaseZombie )
END_DATADESC()

//LINK_ENTITY_TO_CLASS( zmbase_zombie, CZMBaseZombie );

CZMBaseZombie::CZMBaseZombie()
{
    // We have to increment our population in derived classes since we don't know our class yet.
    g_pZombies->AddToTail( this );


    m_iMode = ZOMBIEMODE_OFFENSIVE;
    
    m_bCommanded = false;
    m_flLastCommand = 0.0f;
    m_iSelectorIndex = 0;
    m_flHealthRatio = 1.0f;
    m_flAddGoalTolerance = 0.0f;


    m_hPhysicsEnt.Set( nullptr );

    m_flNextFlinch = 0.0f;
}

CZMBaseZombie::~CZMBaseZombie()
{
    g_pZombies->FindAndRemove( this );



    // It's safe to remove pop count here.
    CZMRules* pRules = ZMRules();

    // Will assert when changing maps.
    //Assert( pRules );

    if ( !pRules ) return;


    pRules->SetZombiePop( pRules->GetZombiePop() - GetPopCost() );
}

void CZMBaseZombie::Spawn( void )
{
    AddSpawnFlags( SF_NPC_FADE_CORPSE );

    SetBloodColor( BLOOD_COLOR_RED );
    


    SetSolid( SOLID_BBOX );
    SetMoveType( MOVETYPE_STEP );

    m_NPCState          = NPC_STATE_NONE;

    m_flNextSwat        = gpGlobals->curtime;
    m_flNextSwatScan    = gpGlobals->curtime;


    CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
    CapabilitiesAdd( bits_CAP_SQUAD );
    

    SetZombieModel();

    NPCInit();


    // Zombies get to cheat for 6 seconds (sjb)
    GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

    m_ActBusyBehavior.SetUseRenderBounds( true );
}

void CZMBaseZombie::Precache()
{
    BaseClass::Precache();


    PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
    PrecacheScriptSound( "NPC_BaseZombie.Swat" );

    PrecacheParticleSystem( "blood_impact_zombie_01" );



    PrecacheScriptSound( "BaseCombatCharacter.CorpseGib" );
    PrecacheScriptSound( "NPC_Antlion.RunOverByVehicle" );
}

void CZMBaseZombie::SetModel( const char* model )
{
    Hull_t lastHull = GetHullType();


    BaseClass::SetModel( model );


    SetHullType( HULL_HUMAN );


    SetHullSizeNormal( true );
    SetDefaultEyeOffset();
    SetActivity( ACT_IDLE );

    // hull changed size, notify vphysics
    // UNDONE: Solve this generally, systematically so other
    // NPCs can change size
    if ( lastHull != GetHullType() )
    {
        if ( VPhysicsGetObject() )
        {
            SetupVPhysicsHull();
        }
    }
}

float CZMBaseZombie::MaxYawSpeed( void )
{
    // ZMRCHANGE: Increased these to ZM 1.2.1 levels.
    if ( IsMoving() && HasPoseParameter( GetSequence(), m_poseMove_Yaw ) )
    {
        return 20.0f;
    }
    else
    {
        switch( GetActivity() )
        {
        case ACT_TURN_LEFT:
        case ACT_TURN_RIGHT:
            return 100.0f;
            break;
        case ACT_RUN:
            return 15.0f;
            break;
        case ACT_WALK:
            return 25.0f;
            break;
        case ACT_IDLE:
            return 35.0f;
            break;
        case ACT_RANGE_ATTACK1:
        case ACT_RANGE_ATTACK2:
        case ACT_MELEE_ATTACK1:
        case ACT_MELEE_ATTACK2:
            return 120.0f;
        default:
            return 90.0f;
            break;
        }
    }
}

bool CZMBaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t& move, float flInterval )
{
    if (!HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
    {
        return BaseClass::OverrideMoveFacing( move, flInterval );
    }

    // required movement direction
    float flMoveYaw = UTIL_VecToYaw( move.dir );
    float idealYaw = UTIL_AngleMod( flMoveYaw );

    if (GetEnemy())
    {
        float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

        if (flEDist < 256.0)
        {
            float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

            if (flEDist < 128.0)
            {
                idealYaw = flEYaw;
            }
            else
            {
                idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
            }

            //DevMsg("was %.0f now %.0f\n", flMoveYaw, idealYaw );
        }
    }

    GetMotor()->SetIdealYawAndUpdate( idealYaw );

    // find movement direction to compensate for not being turned far enough
    float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
    float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
    SetPoseParameter( m_poseMove_Yaw, GetPoseParameter( m_poseMove_Yaw ) + flDiff );

    return true;
}

void CZMBaseZombie::HandleAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == AE_ZOMBIE_POUND )
    {
        PoundSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
    {
        AlertSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
    {
        //MakeAIFootstepSound( 180.0f );
        FootstepSound( false );
        return;
    }
    
    if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
    {
        //MakeAIFootstepSound( 180.0f );
        FootstepSound( true );
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_GET_UP )
    {
        //MakeAIFootstepSound( 180.0f, 3.0f );
        if( !IsOnFire() )
        {
            // If you let this code run while a zombie is burning, it will stop wailing. 
            //m_flNextMoanSound = gpGlobals->curtime;
            //MoanSound();
        }
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
    {
        //MakeAIFootstepSound( 180.0f );
        FootscuffSound( false );
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
    {
        //MakeAIFootstepSound( 180.0f );
        FootscuffSound( true );
        return;
    }

    // all swat animations are handled as a single case.
    if ( pEvent->event == AE_ZOMBIE_STARTSWAT )
    {
        //MakeAIFootstepSound( 180.0f );
        AttackSound();
        return;
    }

    if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
    {
        AttackSound();
        return;
    }

    // Override swatting.
    // Otherwise zombies wouldn't swat without an enemy.
    if ( pEvent->event == AE_ZOMBIE_SWATITEM )
    {
        CBaseEntity* pEnemy = GetEnemy();

        CBaseEntity* pEnt = m_hPhysicsEnt.Get();


        if( !pEnt )
        {
            DevMsg( "**Zombie: Missing my physics ent!!" );
            return;
        }
            
        IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

        if( !pPhys )
        {
            DevMsg( "**Zombie: No Physics Object for physics Ent!" );
            return;
        }

        // Slap that shit out of their hands.
        Pickup_ForcePlayerToDropThisObject( pEnt );


        Vector dir;

        EmitSound( "NPC_BaseZombie.Swat" );

        if ( pEnemy )
        {
            PhysicsImpactSound( pEnemy, pPhys, CHAN_BODY, pPhys->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 800 );

            dir = pEnemy->WorldSpaceCenter() - pEnt->WorldSpaceCenter();
            VectorNormalize( dir );
        }
        else
        {
            AngleVectors( EyeAngles(), &dir, NULL, NULL );
        }

        

        SwatObject( pPhys, dir );


        // If we don't put the object scan time well into the future, the zombie
        // will re-select the object he just hit as it is flying away from him.
        // It will likely always be the nearest object because the zombie moved
        // close enough to it to hit it.
        m_hPhysicsEnt.Set( nullptr );

        m_flNextSwatScan = gpGlobals->curtime + 2.0f;

        return;
    }


#ifdef _DEBUG
    if (pEvent->event == AE_ZOMBIE_ATTACK_BOTH
    ||  pEvent->event == AE_ZOMBIE_ATTACK_LEFT
    ||  pEvent->event == AE_ZOMBIE_ATTACK_RIGHT)
    {
        Warning( "You forgot to implement %s%s%s attack (%i)\n",
            pEvent->event == AE_ZOMBIE_ATTACK_RIGHT ? "right" : "",
            pEvent->event == AE_ZOMBIE_ATTACK_LEFT ? "left" : "",
            pEvent->event == AE_ZOMBIE_ATTACK_BOTH ? "both" : "",
            pEvent->event );
        Assert( 0 );
    }
#endif

    return BaseClass::HandleAnimEvent( pEvent );
}

int CZMBaseZombie::GetSwatActivity( void )
{
    if ( m_bSwatBreakable || !CanSwatPhysicsObjects() )
    {
        m_hPhysicsEnt.Set( nullptr ); // Don't try to swat this again after the melee attack.
        return ACT_MELEE_ATTACK1;
    }

    // Hafta figure out whether to swat with left or right arm.
    // Also hafta figure out whether to swat high or low. (later)
    float   flDot;
    Vector  vecRight, vecDirToObj;

    AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
    
    vecDirToObj = m_hPhysicsEnt->GetLocalOrigin() - GetLocalOrigin();
    VectorNormalize(vecDirToObj);

    // compare in 2D.
    vecRight.z = 0.0;
    vecDirToObj.z = 0.0;

    flDot = DotProduct( vecRight, vecDirToObj );

    Vector vecMyCenter;
    Vector vecObjCenter;

    vecMyCenter = WorldSpaceCenter();
    vecObjCenter = m_hPhysicsEnt->WorldSpaceCenter();
    float flZDiff;

    flZDiff = vecMyCenter.z - vecObjCenter.z;

    if( flDot >= 0 )
    {
        // Right
        if( flZDiff < 0 )
        {
            return ACT_ZOM_SWATRIGHTMID;
        }

        return ACT_ZOM_SWATRIGHTLOW;
    }
    else
    {
        // Left
        if( flZDiff < 0 )
        {
            return ACT_ZOM_SWATLEFTMID;
        }

        return ACT_ZOM_SWATLEFTLOW;
    }
}

float CZMBaseZombie::DistToPhysicsEnt( void )
{
    if ( m_hPhysicsEnt.Get() != nullptr )
        return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );

    return ZOMBIE_PHYSOBJ_SWATDIST + 1;
}

void CZMBaseZombie::StartTask( const Task_t* pTask )
{
    switch( pTask->iTask )
    {
    case TASK_FACE_ENEMY :
        // ALWAYS face the entity if it's a not a normal prop (eg. breakable)
        // Example where you'd want this: zm_ship, to break the masts but the zombies keep facing the enemy even though you're forcing them to attack it.
        if ( m_hPhysicsEnt.Get() )
        {
            IPhysicsObject* pPhys = m_hPhysicsEnt->VPhysicsGetObject();

            if ( !GetEnemy() || !pPhys || !pPhys->IsMoveable() )
            {
                CAI_Motor* motor = GetMotor();

                if ( !motor ) return;


                motor->SetIdealYawToTarget( m_hPhysicsEnt->WorldSpaceCenter() );
                motor->SetIdealYaw( CalcReasonableFacing( true ) );
                motor->SnapYaw();

                TaskComplete();

                break;
            }
        }

        CAI_BaseNPC::StartTask( pTask );
        break;

    case TASK_ZOMBIE_DIE :
        // Go to ragdoll
        KillMe();
        TaskComplete();
        break;

    case TASK_ZM_DEFEND_PATH_TO_DEFPOS :
        GetNavigator()->SetGoal( AI_NavGoal_t( m_vecLastCommandPos ) );
        break;

    case TASK_ZM_SET_TOLERANCE_DISTANCE :
        GetNavigator()->SetGoalTolerance( pTask->flTaskData + m_flAddGoalTolerance );
        TaskComplete();
        break;

    case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ :
    {
        if ( m_hPhysicsEnt.Get() == nullptr )
        {
            TaskFail( "No physics ent!\n" );
            return;
        }

        Vector vecGoalPos;
        Vector vecDir;

        vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
        VectorNormalize(vecDir);
        vecDir.z = 0;

        AI_NavGoal_t goal( m_hPhysicsEnt->WorldSpaceCenter() );
        goal.pTarget = m_hPhysicsEnt.Get();
        GetNavigator()->SetGoal( goal );

        TaskComplete();
    }
    break;

    case TASK_ZOMBIE_SWAT_ITEM :
    {
        if( m_hPhysicsEnt.Get() == nullptr )
        {
            // Physics Object is gone! Probably was an explosive 
            // or something else broke it.
            TaskFail( "Physics ent NULL" );
        }
        else if ( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
        {
            // Physics ent is no longer in range! Probably another zombie swatted it or it moved
            // for some other reason.
            TaskFail( "Physics swat item has moved" );
        }
        else
        {
            SetIdealActivity( (Activity)GetSwatActivity() );
        }
        break;
    }
    break;

    case TASK_ZOMBIE_DELAY_SWAT :
        m_flNextSwat = gpGlobals->curtime + pTask->flTaskData;
        TaskComplete();
        break;

    //case TASK_ZOMBIE_RELEASE_HEADCRAB : // Not used.
    //    TaskComplete();
    //    break;

    case TASK_ZOMBIE_WAIT_POST_MELEE :
    {
#ifndef HL2_EPISODIC
        TaskComplete();
        return;
#endif

        // Don't wait when attacking the player
        if ( GetEnemy() && GetEnemy()->IsPlayer() )
        {
            TaskComplete();
            return;
        }

        // Wait a single think
        SetWait( 0.1 );
    }
    break;

    default:
        BaseClass::StartTask( pTask );
    }
}

void CZMBaseZombie::RunTask( const Task_t* pTask )
{
    switch( pTask->iTask )
    {
    case TASK_ZOMBIE_SWAT_ITEM:
        if( IsActivityFinished() )
        {
            TaskComplete();
        }
        break;

    case TASK_ZOMBIE_WAIT_POST_MELEE:
        {
            if ( IsWaitFinished() )
            {
                TaskComplete();
            }
        }
        break;
    default:
        BaseClass::RunTask( pTask );
        break;
    }
}

void CZMBaseZombie::GatherConditions( void )
{
    //ClearCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION );

    CAI_BaseNPC::GatherConditions();
    

    if ((gpGlobals->curtime - m_flLastCommand) > COMMANDED_SEE_ENEMY_GRACE
    &&  (HasCondition( COND_SEE_HATE ) || HasCondition( COND_SEE_DISLIKE ) || HasCondition( COND_SEE_NEMESIS )))
    {
        SetCondition( COND_ZM_SEE_ENEMY );
    }
    else
    {
        ClearCondition( COND_ZM_SEE_ENEMY );
    }


    if ( GetZombieMode() == ZOMBIEMODE_DEFEND )
    {
        if (GetEnemy()
        &&  GetAbsOrigin().DistToSqr( GetEnemy()->GetAbsOrigin() ) < DEFEND_DIST_CHASENEMY_SQR)
        {
            ClearCondition( COND_ZM_DEFEND_ENEMY_TOOFAR );
            SetCondition( COND_ZM_DEFEND_ENEMY_CLOSE );
        }
        else
        {
            SetCondition( COND_ZM_DEFEND_ENEMY_TOOFAR );
            ClearCondition( COND_ZM_DEFEND_ENEMY_CLOSE );

            SetEnemy( nullptr, false );
        }
    }
    else
    {
        ClearCondition( COND_ZM_DEFEND_ENEMY_TOOFAR );
        ClearCondition( COND_ZM_DEFEND_ENEMY_CLOSE );
    }



    if( m_NPCState == NPC_STATE_COMBAT )
    {
        // This check for !m_pPhysicsEnt prevents a crashing bug, but also
        // eliminates the zombie picking a better physics object if one happens to fall
        // between him and the object he's heading for already. 
        if( !m_hPhysicsEnt.Get() && gpGlobals->curtime >= m_flNextSwatScan )
        {
            FindNearestPhysicsObject( ZOMBIE_MAX_PHYSOBJ_MASS );
            m_flNextSwatScan = gpGlobals->curtime + 2.0;
        }
    }

    if( m_hPhysicsEnt.Get() && gpGlobals->curtime >= m_flNextSwat )
    {
        SetCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
    }
    else
    {
        ClearCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
    }
}

int CZMBaseZombie::TranslateSchedule( int schedule )
{
    switch ( schedule )
    {
        // Always chase the enemy. This was causing problems with fast zombies not wanting to chase us, and instead trying to "shoot" us.
    case SCHED_CHASE_ENEMY :
    case SCHED_MOVE_TO_WEAPON_RANGE : return SCHED_ZOMBIE_CHASE_ENEMY;

    case SCHED_ZOMBIE_SWATITEM:
        // If the object is far away, move and swat it. If it's close, just swat it.
        if( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
        {
            return SCHED_ZOMBIE_MOVE_SWATITEM;
        }
        else
        {
            return SCHED_ZOMBIE_SWATITEM;
        }
    case SCHED_STANDOFF: return SCHED_ZOMBIE_WANDER_STANDOFF;
    case SCHED_MELEE_ATTACK1: return SCHED_ZOMBIE_MELEE_ATTACK1;
    }

    return CAI_BaseNPC::TranslateSchedule( schedule );
}

int CZMBaseZombie::SelectSchedule( void )
{
    if ( BehaviorSelectSchedule() )
    {
        return BaseClass::SelectSchedule();
    }

    if ( GetZombieMode() == ZOMBIEMODE_DEFEND )
    {
        if ( !HasCondition( COND_ZM_DEFEND_ENEMY_CLOSE ) )
        {
            if ( GetLastCommandedPos().DistToSqr( GetAbsOrigin() ) > (48.0f * 48.0f) )
            {
                return SCHED_ZM_DEFEND_GO_DEFPOS;
            }
            else
            {
                return SCHED_ZM_DEFEND_WAIT;
            }
            
        }
    }

    switch ( m_NPCState )
    {
    case NPC_STATE_COMBAT:
        /*if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
        {
            float flDist;

            flDist = ( GetLocalOrigin() - GetEnemy()->GetLocalOrigin() ).Length();

            // If this is a new enemy that's far away, ambush!!
            if (flDist >= zombie_ambushdist.GetFloat() && MustCloseToAttack() )
            {
                return SCHED_ZOMBIE_MOVE_TO_AMBUSH;
            }
        }*/


        // Try to swat before thinking the enemy is unreachable.
        if( HasCondition( COND_ZOMBIE_CAN_SWAT_ATTACK ) )
        {
            return SCHED_ZOMBIE_SWATITEM;
        }

        if ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() )
        {
            return SCHED_ZOMBIE_WANDER_MEDIUM;
        }

        break;

    case NPC_STATE_ALERT:
        if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
        {
            ClearCondition( COND_LOST_ENEMY );
            ClearCondition( COND_ENEMY_UNREACHABLE );

            // Just lost track of our enemy. 
            // Wander around a bit so we don't look like a dingus.
            return SCHED_ZOMBIE_WANDER_MEDIUM;
        }
        break;
    }

    return CAI_BaseNPC::SelectSchedule();
}

int CZMBaseZombie::SelectFailSchedule( int failedSched, int failedTask, AI_TaskFailureCode_t failCode )
{
    // Keep trying!
    if ( ShouldTryScheduleAgain( failedSched, failedTask, failCode ) )
    {
        //ClearCondition( COND_ZM_FAILED_GOAL );
        return failedSched;
    }
    else
    {
        //SetCondition( COND_ZM_FAILED_GOAL );
    }

    return CAI_BaseNPC::SelectFailSchedule( failedSched, failedTask, failCode );
}

bool CZMBaseZombie::OnInsufficientStopDist( AILocalMoveGoal_t* pMoveGoal, float distClear, AIMoveResult_t* pResult )
{
    /*
    if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_NPC )
    {
        CAI_BaseNPC* pNPC = pMoveGoal->directTrace.pObstruction ? pMoveGoal->directTrace.pObstruction->MyNPCPointer() : nullptr;


        return ( pNPC && pNPC->IsMoving() );
    }
    */

    if ( !CanSwatPhysicsObjects() ) return false;


    // Keep trying to go this way if the obstructor is a prop/breakable.
    if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_ENTITY )
    {
        CBaseEntity* pEnt = pMoveGoal->directTrace.pObstruction;

        if ( pEnt )
        {
            IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
            CBreakable* pBreak = dynamic_cast<CBreakable*>( pEnt );
            
            if ((pPhys && pPhys->IsMoveable())
            ||  (pBreak && pBreak->IsBreakable()))
            {
                
                SetSchedule( SCHED_ZOMBIE_SWATITEM );
                
                m_hPhysicsEnt.Set( pEnt );
                m_bSwatBreakable = pBreak ? true : false;
                //m_hObstructor = pEnt;

                return true;
            }
        }
    }
    
    return false;
}

static ConVar zm_sv_swatlift( "zm_sv_swatlift", "20000", FCVAR_NOTIFY );
static ConVar zm_sv_swatforcemin( "zm_sv_swatforcemin", "20000", FCVAR_NOTIFY );
static ConVar zm_sv_swatforcemax( "zm_sv_swatforcemax", "70000", FCVAR_NOTIFY );
static ConVar zm_sv_swatangvel( "zm_sv_swatangvel", "1000", FCVAR_NOTIFY, "Amount of angular velocity swatting applies to prop." );

void CZMBaseZombie::SwatObject( IPhysicsObject* pPhys, Vector& dir )
{
    int mass = pPhys->GetMass();


    Vector uplift = Vector( 0, 0, RemapVal( mass, 5, 350, 3000, zm_sv_swatlift.GetFloat() ) );

    float force = RemapVal( mass, 5, 500, zm_sv_swatforcemin.GetFloat(), zm_sv_swatforcemax.GetFloat() );


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
}

bool CZMBaseZombie::FindNearestPhysicsObject( int iMaxMass )
{
    m_hPhysicsEnt.Set( nullptr );
    m_bSwatBreakable = false;


    if ( !CanSwatPhysicsObjects() )
    {
        return false;
    }


    CBaseEntity* pList[ZOMBIE_PHYSICS_SEARCH_DEPTH];
    CBaseEntity* pNearest = nullptr;
    float flDist;
    IPhysicsObject* pPhysObj;
    int i;
    Vector vecDirToGoal;
    Vector vecDirToObject;
    Vector vecTarget;

    if ( GetEnemy() )
    {
        vecTarget = GetEnemy()->GetAbsOrigin();
    }
    else
    {
        vecTarget = GetNavigator()->GetGoalPos();
    }


    if ( GetAbsOrigin().DistTo( vecTarget ) > ZOMBIE_PLAYER_MAX_SWAT_DIST )
    {
        return false;
    }


    vecDirToGoal = vecTarget - GetAbsOrigin();
    vecDirToGoal.z = 0;
    VectorNormalize( vecDirToGoal );

    if ( GetNavigator()->GetPath()->GetCurWaypoint() )
    {
        Vector vecDirToWaypoint = GetNavigator()->GetCurWaypointPos() - GetAbsOrigin();
        vecDirToWaypoint.z = 0;
        VectorNormalize( vecDirToWaypoint );


        // Don't bother swatting if we have to go the other way!
        if ( DotProduct( vecDirToGoal, vecDirToWaypoint ) < -0.1f )
            return false;
    }


    // If the enemy is closer than prop then don't bother.
    float flNearestDist = GetEnemy() ? GetAbsOrigin().DistTo( vecTarget ) : ZOMBIE_FARTHEST_PHYSICS_OBJECT;

    Vector vecDelta( flNearestDist, flNearestDist, GetHullHeight() * 2.0 );

    class CZombieSwatEntitiesEnum : public CFlaggedEntitiesEnum
    {
    public:
        CZombieSwatEntitiesEnum( CBaseEntity **pList, int listMax, int iMaxMass )
         :	CFlaggedEntitiesEnum( pList, listMax, 0 ),
            m_iMaxMass( iMaxMass )
        {
        }

        virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
        {
            CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
            if ( pEntity && 
                 pEntity->IsSolid() &&
                 pEntity->GetCollisionGroup() != COLLISION_GROUP_WEAPON && // Don't bother trying to swat ammo/weapons.
                 pEntity->VPhysicsGetObject() && 
                 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
                 //pEntity->VPhysicsGetObject()->IsAsleep() && 
                 pEntity->VPhysicsGetObject()->IsMoveable() )
            {
                return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
            }
            return ITERATION_CONTINUE;
        }

        int m_iMaxMass;
    };

    CZombieSwatEntitiesEnum swatEnum( pList, ZOMBIE_PHYSICS_SEARCH_DEPTH, iMaxMass );

    int count = UTIL_EntitiesInBox( GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, &swatEnum );

    // magically know where they are
    Vector vecZombieKnees;
    CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.20f ), &vecZombieKnees );

    for( i = 0 ; i < count ; i++ )
    {
        pPhysObj = pList[ i ]->VPhysicsGetObject();

        Assert( !( !pPhysObj || pPhysObj->GetMass() > iMaxMass ) );

        Vector center = pList[ i ]->WorldSpaceCenter();
        flDist = UTIL_DistApprox2D( GetAbsOrigin(), center );

        if( flDist >= flNearestDist )
            continue;
        

        // This object is closer... but is it between the player and the zombie?
        vecDirToObject = pList[ i ]->WorldSpaceCenter() - GetAbsOrigin();
        vecDirToObject.z = 0;
        VectorNormalize(vecDirToObject);

        if( DotProduct( vecDirToGoal, vecDirToObject ) < 0.8 )
            continue;

        if( flDist >= UTIL_DistApprox2D( center, vecTarget ) )
            continue;

        // don't swat things where the highest point is under my knees
        // NOTE: This is a rough test; a more exact test is going to occur below
        if ( (center.z + pList[i]->BoundingRadius()) < vecZombieKnees.z )
            continue;

        // don't swat things that are over my head.
        if( center.z > EyePosition().z )
            continue;

        vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
        
        Vector objMins, objMaxs;
        physcollision->CollideGetAABB( &objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

        if ( objMaxs.z < vecZombieKnees.z )
            continue;

        if ( !FVisible( pList[i] ) )
            continue;

        // Make this the last check, since it makes a string.
        // Don't swat server ragdolls!
        if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
            continue;
            
        if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
            continue;

        // The object must also be closer to the zombie than it is to the enemy
        pNearest = pList[ i ];
        flNearestDist = flDist;
    }

    m_hPhysicsEnt.Set( pNearest );


    return m_hPhysicsEnt.Get() != nullptr;
}

int CZMBaseZombie::MeleeAttack1Conditions( float flDot, float flDist )
{
    if (flDist > GetClawAttackRange() )
    {
        return COND_TOO_FAR_TO_ATTACK;
    }
    
    if ( flDot < 0.7 )
    {
        return COND_NOT_FACING_ATTACK;
    }


    Vector vecMins, vecMaxs;
    GetAttackHull( vecMins, vecMaxs );

    Vector forward;
    GetVectors( &forward, NULL, NULL );

    trace_t	tr;
    CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
    AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * GetClawAttackRange(), vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

    CBaseEntity* pEnt = tr.m_pEnt;

    if( tr.fraction == 1.0f || !pEnt )
    {
        // This attack would miss completely. Trick the zombie into moving around some more.
        return COND_TOO_FAR_TO_ATTACK;
    }
    

    bool bBreakable = dynamic_cast<CBreakable*>( pEnt ) || dynamic_cast<CBreakableProp*>( pEnt );

    if( pEnt == GetEnemy() || 
        pEnt->IsPlayer() || 
        ( pEnt->m_takedamage == DAMAGE_YES && bBreakable ) )
    {
        // -Let the zombie swipe at his enemy if he's going to hit them.
        // -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
        //  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
        // -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
        return COND_CAN_MELEE_ATTACK1;
    }

    // If we're close enough just force the attack.
    // The hull check may fail if there's something in the way.
    if ( GetEnemy() )
    {
        if ( GetAttackPos().DistTo( GetEnemy()->WorldSpaceCenter() ) <= GetClawAttackRange() )
            return COND_CAN_MELEE_ATTACK1;
    }


    /*Vector vecTrace = tr.endpos - tr.startpos;
    float lenTraceSq = vecTrace.Length2DSqr();

    if( tr.m_pEnt->IsBSPModel() )
    {
        // The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
        // the enemy is, treat this as an obstruction.
        Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

        if( lenTraceSq < vecToEnemy.Length2DSqr() )
        {
            return COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION;
        }
    }

    if ( pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
    {
        //Try to swat whatever the player is standing on instead of acting like a dill.
        return COND_CAN_MELEE_ATTACK1;
    }*/

    // Bullseyes are given some grace on if they can be hit
    if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
        return COND_CAN_MELEE_ATTACK1;


    // Move around some more
    return COND_TOO_FAR_TO_ATTACK;
}

void CZMBaseZombie::GetAttackHull( Vector& mins, Vector& maxs )
{
    // Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
    mins = GetHullMins();
    maxs = GetHullMaxs();

    mins.z = -(( EyePosition().z - GetAbsOrigin().z ) * 0.5f);
    // Let zombies attack higher than their head. (enemies right on top of them.)
    maxs.z *= 0.6f;
}

const Vector CZMBaseZombie::GetAttackPos() const
{
    Vector pos = GetAbsOrigin();
    pos.z += GetHullHeight() * 0.7f; // About shoulder height.

    return pos;
}

#define ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST	96.0f // Triple damage from buckshot at 8 feet (headshot only)
float CZMBaseZombie::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo& info )
{
    switch( iHitGroup )
    {
    case HITGROUP_HEAD:
        {
            if( info.GetDamageType() & DMG_BUCKSHOT )
            {
                float flDist = FLT_MAX;

                if( info.GetAttacker() )
                {
                    flDist = ( GetAbsOrigin() - info.GetAttacker()->GetAbsOrigin() ).Length();
                }

                if( flDist <= ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST )
                {
                    return 3.0f;
                }
            }
            else
            {
                return 2.0f;
            }
        }
    }

    return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}

void CZMBaseZombie::TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator )
{
    CTakeDamageInfo infoCopy = info;


    if( infoCopy.GetDamageType() & DMG_BUCKSHOT )
    {
        // Zombie gets across-the-board damage reduction for buckshot. This compensates for the recent changes which
        // make the shotgun much more powerful, and returns the zombies to a level that has been playtested extensively.(sjb)
        // This normalizes the buckshot damage to what it used to be on normal (5 dmg per pellet. Now it's 8 dmg per pellet). 
        infoCopy.ScaleDamage( 0.625 );
    }

    BaseClass::TraceAttack( infoCopy, vecDir, ptr, pAccumulator );
}

#define ZOMBIE_SCORCH_RATE		8
#define ZOMBIE_MIN_RENDERCOLOR	50
int CZMBaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo& inputInfo )
{
    CTakeDamageInfo info = inputInfo;

    if( info.GetDamageType() & DMG_BURN )
    {
        // If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
        // This is to stop zombies from burning to death 10x faster when they're standing around
        // 10 fire entities.
        if( IsOnFire() && !(info.GetDamageType() & DMG_DIRECT) )
        {
            return 0;
        }
        
        Scorch( ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR );
    }

    if ( ShouldIgnite( info ) )
    {
        Ignite( 100.0f );
    }

    // Take some percentage of damage from bullets (unless hit in the crab). Always take full buckshot & sniper damage
    if ( (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & (DMG_BUCKSHOT/*|DMG_SNIPER*/)) )
    {
        info.ScaleDamage( ZOMBIE_BULLET_DAMAGE_SCALE );
    }

    int tookDamage = BaseClass::OnTakeDamage_Alive( info );

    if ( tookDamage )
    {
        m_flHealthRatio = m_iHealth / (float)(m_iMaxHealth > 0 ? m_iMaxHealth : 1);
    }

    // flDamageThreshold is what percentage of the creature's max health
    // this amount of damage represents. (clips at 1.0)
    /*float flDamageThreshold = MIN( 1, info.GetDamage() / m_iMaxHealth );

   if( tookDamage > 0 && (info.GetDamageType() & (DMG_BURN|DMG_DIRECT)) && m_ActBusyBehavior.IsActive() ) 
    {
        //!!!HACKHACK- Stuff a light_damage condition if an actbusying zombie takes direct burn damage. This will cause an
        // ignited zombie to 'wake up' and rise out of its actbusy slump. (sjb)
        SetCondition( COND_LIGHT_DAMAGE );
    }*/

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
        m_flBurnDamage += info.GetDamage();
        m_flBurnDamageResetTime = gpGlobals->curtime + 5;

        if ( m_flBurnDamage >= m_iMaxHealth * 0.1 )
        {
            return true;
        }
    }

    return false;
}

/*void CZMBaseZombie::MakeAISpookySound( float volume, float duration )
{

}*/
/*
bool CZMBaseZombie::CanPlayMoanSound()
{
    if( HasSpawnFlags( SF_NPC_GAG ) )
        return false;

    // Burning zombies play their moan loop at full volume for as long as they're
    // burning. Don't let a moan envelope play cause it will turn the volume down when done.
    if( IsOnFire() )
        return false;

    // Members of a small group of zombies can vocalize whenever they want
    if( s_iAngryZombies <= 4 )
        return true;

    // This serves to limit the number of zombies that can moan at one time when there are a lot. 
    if( random->RandomInt( 1, zombie_moanfreq.GetInt() * (s_iAngryZombies/2) ) == 1 )
    {
        return true;
    }

    return false;
}

void CZMBaseZombie::MoanSound()
{
    if( HasSpawnFlags( SF_NPC_GAG ) )
    {
        // Not yet!
        return;
    }

    if( !m_pMoanSound )
    {
        // Don't set this up until the code calls for it.
        const char *pszSound = GetMoanSound( m_iMoanSound );
        m_flMoanPitch = random->RandomInt( zombie_basemin.GetInt(), zombie_basemax.GetInt() );

        //m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
        CPASAttenuationFilter filter( this );
        m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, ATTN_NORM );

        ENVELOPE_CONTROLLER.Play( m_pMoanSound, 1.0, m_flMoanPitch );
    }

    //HACKHACK get these from chia chin's console vars.
    //envDefaultZombieMoanVolumeFast[ 1 ].durationMin = zombie_decaymin.GetFloat();
    //envDefaultZombieMoanVolumeFast[ 1 ].durationMax = zombie_decaymax.GetFloat();

    if( random->RandomInt( 1, 2 ) == 1 )
    {
        IdleSound();
    }

    float duration = 0.0f;//ENVELOPE_CONTROLLER.Play( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

    float flPitch = random->RandomInt( m_flMoanPitch + zombie_changemin.GetInt(), m_flMoanPitch + zombie_changemax.GetInt() );
    ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, flPitch, 0.3 );

    m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}
*/
void CZMBaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
    BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );


    // Set the zombie up to burn to death in about ten seconds.
    SetHealth( MIN( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat( -ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE)) ) );

    // FIXME: use overlays when they come online
    //AddOverlay( ACT_ZOM_WALK_ON_FIRE, false );
    /*if ( !m_ActBusyBehavior.IsActive() )
    {
        Activity activity = GetActivity();
        Activity burningActivity = activity;

        if ( activity == ACT_WALK )
        {
            burningActivity = ACT_WALK_ON_FIRE;
        }
        else if ( activity == ACT_RUN )
        {
            burningActivity = ACT_RUN_ON_FIRE;
        }
        else if ( activity == ACT_IDLE )
        {
            burningActivity = ACT_IDLE_ON_FIRE;
        }

        if( HaveSequenceForActivity(burningActivity) )
        {
            // Make sure we have a sequence for this activity (torsos don't have any, for instance) 
            // to prevent the baseNPC & baseAnimating code from throwing red level errors.
            SetActivity( burningActivity );
        }
    }*/
}

CBaseEntity* CZMBaseZombie::ClawAttack( float flDist, int iDamage, const QAngle& qaViewPunch, const Vector& vecVelocityPunch, int BloodOrigin )
{
    // Added test because claw attack anim sometimes used when for cases other than melee
    int iDriverInitialHealth = -1;
    CBaseEntity *pDriver = NULL;
    if ( GetEnemy() )
    {
        trace_t	tr;
        AI_TraceLine( GetAttackPos(), GetEnemy()->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

        if ( tr.fraction < 1.0f )
            return nullptr;
    }

    //
    // Trace out a cubic section of our hull and see what we hit.
    //
    Vector vecMins, vecMaxs;
    GetAttackHull( vecMins, vecMaxs );


    CBaseEntity *pHurt = NULL;
    if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
    { 
        // We always hit bullseyes we're targeting
        pHurt = GetEnemy();
        CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), iDamage, DMG_SLASH );
        pHurt->TakeDamage( info );
    }
    else 
    {
        // Try to hit them with a trace
        pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_SLASH );
    }

    if ( pDriver && iDriverInitialHealth != pDriver->GetHealth() )
    {
        pHurt = pDriver;
    }

    if ( !pHurt && m_hPhysicsEnt.Get() && IsCurSchedule( SCHED_ZOMBIE_ATTACKITEM ) )
    {
        pHurt = m_hPhysicsEnt.Get();

        Vector vForce = pHurt->WorldSpaceCenter() - WorldSpaceCenter(); 
        VectorNormalize( vForce );

        vForce *= 5 * 24;

        CTakeDamageInfo info( this, this, vForce, GetAbsOrigin(), iDamage, DMG_SLASH );
        pHurt->TakeDamage( info );
    }

    if ( pHurt )
    {
        AttackHitSound();

        CBasePlayer *pPlayer = ToBasePlayer( pHurt );

        if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE ) )
        {
            pPlayer->ViewPunch( qaViewPunch );
            
            pPlayer->VelocityPunch( vecVelocityPunch );
        }
        else if( !pPlayer && UTIL_ShouldShowBlood( pHurt->BloodColor() ) )
        {
            // Hit an NPC. Bleed them!
            Vector vecBloodPos;

            switch( BloodOrigin )
            {
            case ZOMBIE_BLOOD_LEFT_HAND:
                if( GetAttachment( "blood_left", vecBloodPos ) )
                    SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
                break;

            case ZOMBIE_BLOOD_RIGHT_HAND:
                if( GetAttachment( "blood_right", vecBloodPos ) )
                    SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
                break;

            case ZOMBIE_BLOOD_BOTH_HANDS:
                if( GetAttachment( "blood_left", vecBloodPos ) )
                    SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );

                if( GetAttachment( "blood_right", vecBloodPos ) )
                    SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN( iDamage, 30 ) );
                break;

            case ZOMBIE_BLOOD_BITE:
                // No blood for these.
                break;
            }
        }
    }
    else 
    {
        AttackMissSound();
    }

    if ( pHurt == m_hPhysicsEnt.Get() && IsCurSchedule( SCHED_ZOMBIE_ATTACKITEM ) )
    {
        m_hPhysicsEnt.Set( nullptr );
        m_flNextSwat = gpGlobals->curtime + random->RandomFloat( 2, 4 );
    }

    return pHurt;
}

void CZMBaseZombie::PoundSound()
{
    trace_t		tr;
    Vector		forward;

    GetVectors( &forward, NULL, NULL );

    AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

    if ( tr.fraction == 1.0f ) return;


    if ( tr.fraction < 1.0 && tr.m_pEnt )
    {
        const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
        if ( psurf )
        {
            EmitSound( physprops->GetString( psurf->sounds.impactHard ) );
            return;
        }
    }

    // Otherwise fall through to the default sound.
    CPASAttenuationFilter filter( this, "NPC_BaseZombie.PoundDoor" );
    EmitSound( filter, entindex(), "NPC_BaseZombie.PoundDoor" );
}

void CZMBaseZombie::BuildScheduleTestBits( void )
{
    // Ignore damage if we were recently damaged or we're attacking.
    if ( GetActivity() == ACT_MELEE_ATTACK1 )
    {
        ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
        ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
    }
#ifndef HL2_EPISODIC
    else if ( m_flNextFlinch >= gpGlobals->curtime )
    {
        ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
        ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
    }
#endif // !HL2_EPISODIC

    // Everything should be interrupted if we get killed.
    //SetCustomInterruptCondition( COND_ZOMBIE_RELEASECRAB );

    BaseClass::BuildScheduleTestBits();
}

void CZMBaseZombie::OnScheduleChange( void )
{
    //
    // If we took damage and changed schedules, ignore further damage for a few seconds.
    //
    if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
    {
        m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;
    } 

    BaseClass::OnScheduleChange();
}

void CZMBaseZombie::PrescheduleThink( void )
{
    BaseClass::PrescheduleThink();

    //
    // Cool off if we aren't burned for five seconds or so. 
    //
    if ( ( m_flBurnDamageResetTime ) && gpGlobals->curtime >= m_flBurnDamageResetTime )
    {
        m_flBurnDamage = 0;
    }
}

/*void CZMBaseZombie::StopLoopingSounds()
{
    //ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
    //m_pMoanSound = nullptr;

    BaseClass::StopLoopingSounds();
}*/

void CZMBaseZombie::OnStateChange( NPC_STATE oldState, NPC_STATE newState )
{
    switch( newState )
    {
    case NPC_STATE_COMBAT:
        RemoveSpawnFlags( SF_NPC_GAG );
        break;
    default: break;
    }

    BaseClass::OnStateChange( oldState, newState );
}

Activity CZMBaseZombie::NPC_TranslateActivity( Activity baseAct )
{
    if ( baseAct == ACT_WALK && IsCurSchedule( SCHED_COMBAT_PATROL, false ) )
        baseAct = ACT_RUN;

    if ( IsOnFire() )
    {
        switch ( baseAct )
        {
            case ACT_RUN_ON_FIRE:
            {
                return ( Activity )ACT_WALK_ON_FIRE;
            }

            case ACT_WALK:
            {
                // I'm on fire. Put ME out.
                return ( Activity )ACT_WALK_ON_FIRE;
            }

            case ACT_IDLE:
            {
                // I'm on fire. Put ME out.
                return ( Activity )ACT_IDLE_ON_FIRE;
            }
        }
    }

    return BaseClass::NPC_TranslateActivity( baseAct );
}

bool CZMBaseZombie::IsValidEnemy( CBaseEntity* pEnt )
{
    CZMPlayer* pPlayer = ToZMPlayer( pEnt );

    if ( pPlayer )
    {
        if ( !pPlayer->IsAlive() ) return false;

        if ( pPlayer->IsZM() ) return false;
    }

    return BaseClass::IsValidEnemy( pEnt );
}

bool CZMBaseZombie::CorpseGib( const CTakeDamageInfo &info )
{
    EmitSound( "BaseCombatCharacter.CorpseGib" );
    EmitSound( "NPC_Antlion.RunOverByVehicle" );

    // ZMRTODO: Better gibbing.
    CGib::SpawnHeadGib( this );


    return true;
}

bool CZMBaseZombie::ShouldGib( const CTakeDamageInfo& info )
{
    if ( info.GetDamageType() & DMG_ALWAYSGIB ) return true;


    return BaseClass::ShouldGib( info );
}

void CZMBaseZombie::SetZombieMode( ZombieMode_t mode )
{
    if ( !HasBeenCommanded() )
    {
        m_vecLastCommandPos = GetAbsOrigin();
        m_bCommanded = true;
    }

    /*switch ( mode )
    {
    case ZOMBIEMODE_DEFEND :
        SetSchedule( SCHED_ZM_ )
    }*/

    m_iMode = mode;
}

void CZMBaseZombie::Command( const Vector& pos, bool bPlayerCommanded, float tolerance )
{
    m_vecLastPosition = pos;

    //AI_NavGoal_t goal;
    //goal.dest = pos;
    
    //GetNavigator()->SetGoal( goal );
    //SetCommandGoal( pos );

    //GetNavigator()->SetGoalTolerance( 128.0f );

    // This allows rally points to work without having to fuck around with the base class.
    if ( GetState() <= NPC_STATE_NONE )
    {
        SetState( NPC_STATE_ALERT );
    }
    

    if ( bPlayerCommanded )
    {
        SetCondition( COND_RECEIVED_ORDERS );
        SetSchedule( SCHED_ZM_FORCED_GO );
    }
    else
    {
        ClearCondition( COND_RECEIVED_ORDERS );
        SetSchedule( SCHED_ZM_GO );
    }

    m_flLastCommand = gpGlobals->curtime;
    m_vecLastCommandPos = pos;
    m_bCommanded = true;
    m_flAddGoalTolerance = tolerance;
}

bool CZMBaseZombie::Swat( CBaseEntity* pTarget, bool bBreakable )
{
    if ( !pTarget ) return false;

    if ( !CanSwatPhysicsObjects() && !bBreakable ) return false;


    m_hPhysicsEnt.Set( pTarget );

    m_bSwatBreakable = bBreakable;

    
    SetSchedule( SCHED_ZOMBIE_SWATITEM );

    return true;
}

bool CZMBaseZombie::TargetEnemy( CBaseEntity* pTarget )
{
    if ( !pTarget ) return false;

    if ( !pTarget->IsPlayer() && !pTarget->IsNPC() ) return false;


    SetEnemy( pTarget, true );

    return true;
}


bool CZMBaseZombie::CanSpawn( const Vector& pos )
{
    trace_t trace;
    Vector up = pos + Vector( 0.0f, 0.0f, 1.0f );

    UTIL_TraceHull( pos, up, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &trace );


    return trace.fraction == 1.0f;
}

bool CZMBaseZombie::ShouldTryScheduleAgain( int failedSched, int failedTask, AI_TaskFailureCode_t failCode )
{
    // We can't build path to position, definitely don't want to try again.
    if ( failedTask == TASK_GET_PATH_TO_LASTPOSITION )
        return false;

    // If we have been commanded (incl. rally points), keep trying.
    switch ( failedSched )
    {
    case SCHED_ZM_FORCED_GO :
    case SCHED_ZM_GO :
        if ( IsPathTaskFailure( failCode ) || failedTask == TASK_WAIT_FOR_MOVEMENT )
        {
            if ( m_vecLastCommandPos.DistToSqr( GetAbsOrigin() ) > max( 48.0f * 48.0f, Square( m_flAddGoalTolerance ) ) )
                return true;
        }
        break;
    default : break;
    }

    return false;
}

AI_BEGIN_CUSTOM_NPC( zmbase_zombie, CZMBaseZombie )
    
    DECLARE_TASK( TASK_ZOMBIE_DELAY_SWAT )
    DECLARE_TASK( TASK_ZOMBIE_SWAT_ITEM )
    DECLARE_TASK( TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ )
    DECLARE_TASK( TASK_ZOMBIE_DIE )
    //DECLARE_TASK( TASK_ZOMBIE_RELEASE_HEADCRAB )
    DECLARE_TASK( TASK_ZOMBIE_WAIT_POST_MELEE )

    DECLARE_TASK( TASK_ZM_DEFEND_PATH_TO_DEFPOS );
    DECLARE_TASK( TASK_ZM_SET_TOLERANCE_DISTANCE );


    DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTMID )
    DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTMID )
    DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTLOW )
    DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTLOW )
    DECLARE_ACTIVITY( ACT_ZOM_RELEASECRAB )
    DECLARE_ACTIVITY( ACT_ZOM_FALL )
    

    DECLARE_CONDITION( COND_ZOMBIE_CAN_SWAT_ATTACK )
    DECLARE_CONDITION( COND_ZM_SEE_ENEMY ) // Used to interrupt ZM command.

    DECLARE_CONDITION( COND_ZM_DEFEND_ENEMY_CLOSE )
    DECLARE_CONDITION( COND_ZM_DEFEND_ENEMY_TOOFAR )
    //DECLARE_CONDITION( COND_ZM_FAILED_GOAL )


    DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH )
    DECLARE_ANIMEVENT( AE_ZOMBIE_SWATITEM )
    DECLARE_ANIMEVENT( AE_ZOMBIE_STARTSWAT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_LEFT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_RIGHT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_LEFT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_RIGHT )
    DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_SCREAM )
    DECLARE_ANIMEVENT( AE_ZOMBIE_GET_UP )
    DECLARE_ANIMEVENT( AE_ZOMBIE_POUND )
    DECLARE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND )
    //DECLARE_ANIMEVENT( AE_ZOMBIE_POPHEADCRAB )
    

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_CHASE_ENEMY,

        "	Tasks"
        "		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
        "		 TASK_SET_TOLERANCE_DISTANCE	24"
        "		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
        "		 TASK_RUN_PATH					0"
        "		 TASK_WAIT_FOR_MOVEMENT			0"
        "		 TASK_FACE_ENEMY				0"
        "	"
        "	Interrupts"
        "		COND_ZM_DEFEND_ENEMY_TOOFAR" // ZMRCHANGE
        "		COND_NEW_ENEMY"
        "		COND_ENEMY_DEAD"
        "		COND_ENEMY_UNREACHABLE"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
        "		COND_TOO_CLOSE_TO_ATTACK"
        "		COND_TASK_FAILED"
        "		COND_ZOMBIE_CAN_SWAT_ATTACK"
        //"		COND_ZOMBIE_RELEASECRAB"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_MOVE_SWATITEM,

        "	Tasks"
        "		TASK_ZOMBIE_DELAY_SWAT			3"
        "		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
        "		TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ	0"
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        "		TASK_FACE_ENEMY					0"
        "		TASK_ZOMBIE_SWAT_ITEM			0"
        "	"
        "	Interrupts"
        "		COND_ZM_DEFEND_ENEMY_TOOFAR" // ZMRCHANGE
        "		COND_ENEMY_DEAD"
        "		COND_NEW_ENEMY"
    )

    //=========================================================
    // SwatItem
    //=========================================================
    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_SWATITEM,

        "	Tasks"
        "		TASK_ZOMBIE_DELAY_SWAT			3"
        "		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
        "		TASK_FACE_ENEMY					0"
        "		TASK_ZOMBIE_SWAT_ITEM			0"
        "	"
        "	Interrupts"
        "		COND_ZM_DEFEND_ENEMY_TOOFAR" // ZMRCHANGE
        "		COND_ENEMY_DEAD"
        "		COND_NEW_ENEMY"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_ATTACKITEM,

        "	Tasks"
        "		TASK_FACE_ENEMY					0"
        "		TASK_MELEE_ATTACK1				0"
        "	"
        "	Interrupts"
        //"		COND_ZOMBIE_RELEASECRAB"
        "		COND_ENEMY_DEAD"
        "		COND_NEW_ENEMY"
    )

    /*DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_RELEASECRAB,

        "	Tasks"
        "		TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ZOM_RELEASECRAB"
        "		TASK_ZOMBIE_RELEASE_HEADCRAB				0"
        "		TASK_ZOMBIE_DIE								0"
        "	"
        "	Interrupts"
        "		COND_TASK_FAILED"
    )*/

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_MOVE_TO_AMBUSH,

        "	Tasks"
        "		TASK_WAIT						1.0" // don't react as soon as you see the player.
        "		TASK_FIND_COVER_FROM_ENEMY		0"
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        "		TASK_STOP_MOVING				0"
        "		TASK_TURN_LEFT					180"
        "		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WAIT_AMBUSH"
        "	"
        "	Interrupts"
        "		COND_TASK_FAILED"
        "		COND_NEW_ENEMY"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_WAIT_AMBUSH,

        "	Tasks"
        "		TASK_WAIT_FACE_ENEMY	99999"
        "	"
        "	Interrupts"
        "		COND_NEW_ENEMY"
        "		COND_SEE_ENEMY"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_WANDER_MEDIUM,

        "	Tasks"
        "		TASK_STOP_MOVING				0"
        "		TASK_WANDER						480384" // 4 feet to 32 feet
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        "		TASK_STOP_MOVING				0"
        "		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
        "		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
        "	"
        "	Interrupts"
        "		COND_NEW_ENEMY"
        "		COND_SEE_ENEMY"
        "		COND_LIGHT_DAMAGE"
        "		COND_HEAVY_DAMAGE"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_WANDER_FAIL,

        "	Tasks"
        "		TASK_STOP_MOVING		0"
        "		TASK_WAIT				1"
        "		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
        "	Interrupts"
        "		COND_NEW_ENEMY"
        "		COND_LIGHT_DAMAGE"
        "		COND_HEAVY_DAMAGE"
        "		COND_ENEMY_DEAD"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
        //"		COND_ZOMBIE_RELEASECRAB"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_WANDER_STANDOFF,

        "	Tasks"
        "		TASK_STOP_MOVING				0"
        "		TASK_WANDER						480384" // 4 feet to 32 feet
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        "		TASK_STOP_MOVING				0"
        "		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
        "	"
        "	Interrupts"
        "		COND_NEW_ENEMY"
        "		COND_LIGHT_DAMAGE"
        "		COND_HEAVY_DAMAGE"
        "		COND_ENEMY_DEAD"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
        //"		COND_ZOMBIE_RELEASECRAB"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_MELEE_ATTACK1,

        "	Tasks"
        "		TASK_STOP_MOVING		0"
        "		TASK_FACE_ENEMY			0"
        "		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
        "		TASK_MELEE_ATTACK1		0"
        "		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_POST_MELEE_WAIT"
        ""
        "	Interrupts"
        "		COND_LIGHT_DAMAGE"
        "		COND_HEAVY_DAMAGE"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZOMBIE_POST_MELEE_WAIT,

        "	Tasks"
        "		TASK_ZOMBIE_WAIT_POST_MELEE		0"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZM_GO, // Used for rallypoints, etc. when going after the enemy is the priority instead of literally getting to your destination.

        "	Tasks"
        "		TASK_SET_TOLERANCE_DISTANCE		48"
        "		TASK_SET_ROUTE_SEARCH_TIME		3"
        "		TASK_GET_PATH_TO_LASTPOSITION	0"
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        ""
        "	Interrupts"
        "		COND_NEW_ENEMY"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
        "		COND_NOT_FACING_ATTACK" // If we're in attack distance but just not facing
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZM_FORCED_GO, // Used by ZM to command a zombie. Will ignore enemies for a while.

        "	Tasks"
        "		TASK_ZM_SET_TOLERANCE_DISTANCE  48"
        "		TASK_SET_ROUTE_SEARCH_TIME		3"
        "		TASK_GET_PATH_TO_LASTPOSITION	0"
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        ""
        "	Interrupts"
        "		COND_ZM_SEE_ENEMY"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZM_DEFEND_GO_DEFPOS,

        "	Tasks"
        "		TASK_SET_TOLERANCE_DISTANCE		48"
        "		TASK_SET_ROUTE_SEARCH_TIME		3"
        "		TASK_ZM_DEFEND_PATH_TO_DEFPOS	0"
        "		TASK_WALK_PATH					0"
        "		TASK_WAIT_FOR_MOVEMENT			0"
        ""
        "	Interrupts"
        "		COND_ZM_DEFEND_ENEMY_CLOSE"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
    )

    DEFINE_SCHEDULE
    (
        SCHED_ZM_DEFEND_WAIT,

        "	Tasks"
        "		TASK_STOP_MOVING				0"
        "		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
        "		TASK_WAIT_PVS					0"
        ""
        "	Interrupts"
        "		COND_ZM_DEFEND_ENEMY_CLOSE"
        "		COND_CAN_RANGE_ATTACK1"
        "		COND_CAN_MELEE_ATTACK1"
        "		COND_CAN_RANGE_ATTACK2"
        "		COND_CAN_MELEE_ATTACK2"
    )

AI_END_CUSTOM_NPC()
