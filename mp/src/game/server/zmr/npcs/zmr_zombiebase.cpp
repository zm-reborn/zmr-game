#include "cbase.h"
#include "npcevent.h"
#include "gib.h"
#include "func_break.h"


#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"

#include "zmr_zombiebase.h"



#define ZOMBIE_PHYSOBJ_SWATDIST         80

#define ZOMBIE_MAX_PHYSOBJ_MASS         200

#define ZOMBIE_PLAYER_MAX_SWAT_DIST     1000

#define ZOMBIE_PHYSICS_SEARCH_DEPTH     32
#define ZOMBIE_FARTHEST_PHYSICS_OBJECT  128


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
    m_fIsHeadless = false;
    m_fIsTorso = false;

    AddSpawnFlags( SF_NPC_FADE_CORPSE );

    SetBloodColor( BLOOD_COLOR_RED );
    
    //CAI_Senses* pSenses = GetSenses();
    BaseClass::Spawn();
}

void CZMBaseZombie::Precache()
{
    BaseClass::Precache();


    PrecacheScriptSound( "BaseCombatCharacter.CorpseGib" );
    PrecacheScriptSound( "NPC_Antlion.RunOverByVehicle" );
}

void CZMBaseZombie::HandleAnimEvent( animevent_t* pEvent )
{
    // Override swatting.
    // Otherwise zombies wouldn't swat without an enemy.
	if ( pEvent->event == AE_ZOMBIE_SWATITEM )
	{
		CBaseEntity* pEnemy = GetEnemy();

		CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;


		if( !pPhysicsEntity )
		{
			DevMsg( "**Zombie: Missing my physics ent!!" );
			return;
		}
			
		IPhysicsObject* pPhys = pPhysicsEntity->VPhysicsGetObject();

		if( !pPhys )
		{
			DevMsg( "**Zombie: No Physics Object for physics Ent!" );
			return;
		}


        Vector dir;

		EmitSound( "NPC_BaseZombie.Swat" );

        if ( pEnemy )
        {
            PhysicsImpactSound( pEnemy, pPhys, CHAN_BODY, pPhys->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 800 );

            dir = pEnemy->WorldSpaceCenter() - pPhysicsEntity->WorldSpaceCenter();
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
		m_hPhysicsEnt = nullptr;

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

int CZMBaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
    int ret = BaseClass::OnTakeDamage_Alive( info );

    if ( ret )
    {
        m_flHealthRatio = m_iHealth / (float)(m_iMaxHealth > 0 ? m_iMaxHealth : 1);
    }

    return ret;
}

int CZMBaseZombie::GetSwatActivity( void )
{
    if ( m_bSwatBreakable || !CanSwatPhysicsObjects() )
    {
        m_hPhysicsEnt = nullptr; // Don't try to swat this again after the melee attack.
        return ACT_MELEE_ATTACK1;
    }

    // Hafta figure out whether to swat with left or right arm.
    // Also hafta figure out whether to swat high or low. (later)
    float		flDot;
    Vector		vecRight, vecDirToObj;

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
            return CNPC_BaseZombie::ACT_ZOM_SWATRIGHTMID;
        }

        return CNPC_BaseZombie::ACT_ZOM_SWATRIGHTLOW;
    }
    else
    {
        // Left
        if( flZDiff < 0 )
        {
            return CNPC_BaseZombie::ACT_ZOM_SWATLEFTMID;
        }

        return CNPC_BaseZombie::ACT_ZOM_SWATLEFTLOW;
    }
}

void CZMBaseZombie::StartTask( const Task_t* pTask )
{
    switch( pTask->iTask )
    {
    case TASK_FACE_ENEMY :
        // ALWAYS face the entity if it's a not a normal prop (eg. breakable)
        // Example where you'd want this: zm_ship, to break the masts but the zombies keep facing the enemy even though you're forcing them to attack it.
        if ( m_hPhysicsEnt )
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

    case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ :
    {
        if ( !m_hPhysicsEnt ) { TaskFail( "No physics ent!\n" ); return; }
        Vector vecGoalPos;
        Vector vecDir;

        vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
        VectorNormalize(vecDir);
        vecDir.z = 0;

        AI_NavGoal_t goal( m_hPhysicsEnt->WorldSpaceCenter() );
        goal.pTarget = m_hPhysicsEnt;
        GetNavigator()->SetGoal( goal );

        TaskComplete();
    }
    break;

    case TASK_ZOMBIE_SWAT_ITEM :
    {
        if( m_hPhysicsEnt == NULL )
        {
            // Physics Object is gone! Probably was an explosive 
            // or something else broke it.
            TaskFail("Physics ent NULL");
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

    case TASK_ZOMBIE_RELEASE_HEADCRAB : // Not used.
        TaskComplete();
        break;

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

void CZMBaseZombie::GatherConditions( void )
{
    ClearCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION );

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
        if( !m_hPhysicsEnt && gpGlobals->curtime >= m_flNextSwatScan )
        {
            FindNearestPhysicsObject( ZOMBIE_MAX_PHYSOBJ_MASS );
            m_flNextSwatScan = gpGlobals->curtime + 2.0;
        }
    }

    if( m_hPhysicsEnt && gpGlobals->curtime >= m_flNextSwat )
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

        if ( HasCondition( COND_LOST_ENEMY ) )
        {
            return SCHED_ZOMBIE_WANDER_MEDIUM;
        }

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

bool CZMBaseZombie::OnInsufficientStopDist( AILocalMoveGoal_t* pMoveGoal, float distClear, AIMoveResult_t* pResult )
{
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
                
                m_hPhysicsEnt = pEnt;
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
    m_hPhysicsEnt = nullptr;
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


    vecDirToGoal = vecTarget - GetAbsOrigin();
    vecDirToGoal.z = 0;
    VectorNormalize( vecDirToGoal );


    if( vecDirToGoal.Length2D() > ZOMBIE_PLAYER_MAX_SWAT_DIST )
    {
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

    m_hPhysicsEnt = pNearest;


    return m_hPhysicsEnt != nullptr;
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

    // Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
    Vector vecMins = GetHullMins();
    Vector vecMaxs = GetHullMaxs();
    vecMins.z = vecMins.x;
    vecMaxs.z = vecMaxs.x;

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

bool CZMBaseZombie::CanBecomeLiveTorso()
{
    return false;
}

bool CZMBaseZombie::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
    return false;
}

HeadcrabRelease_t CZMBaseZombie::ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold )
{
    return RELEASE_NO;
}

bool CZMBaseZombie::CorpseGib( const CTakeDamageInfo &info )
{
    EmitSound( "BaseCombatCharacter.CorpseGib" );
    EmitSound( "NPC_Antlion.RunOverByVehicle" );

    // ZMRTODO: Better gibbing.
    CGib::SpawnHeadGib( this );


    return true;
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

bool CZMBaseZombie::ShouldGib( const CTakeDamageInfo& info )
{
    if ( info.GetDamageType() & DMG_ALWAYSGIB ) return true;


    return BaseClass::ShouldGib( info );
}

bool CZMBaseZombie::IsChopped( const CTakeDamageInfo &info )
{
    return false;
}

void CZMBaseZombie::SetZombieModel( void )
{
    
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

void CZMBaseZombie::Command( const Vector& pos, bool bPlayerCommanded )
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
}

bool CZMBaseZombie::Swat( CBaseEntity* pTarget, bool bBreakable )
{
    if ( !pTarget ) return false;

    if ( !CanSwatPhysicsObjects() && !bBreakable ) return false;


    m_hPhysicsEnt = pTarget;

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


AI_BEGIN_CUSTOM_NPC( zmbase_zombie, CZMBaseZombie )
    
    DECLARE_TASK( TASK_ZM_DEFEND_PATH_TO_DEFPOS );
    
    DECLARE_CONDITION( COND_ZM_SEE_ENEMY ) // Used to interrupt ZM command.

    DECLARE_CONDITION( COND_ZM_DEFEND_ENEMY_CLOSE )
    DECLARE_CONDITION( COND_ZM_DEFEND_ENEMY_TOOFAR )
    

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
	    "		TASK_SET_TOLERANCE_DISTANCE		48"
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
