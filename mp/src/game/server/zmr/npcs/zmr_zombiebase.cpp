#include "cbase.h"
#include "NPCevent.h"


#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"

#include "zmr_zombiebase.h"



//ZombieList_t g_Zombies;

#define ZOMBIE_PHYSOBJ_SWATDIST         80


IMPLEMENT_SERVERCLASS_ST( CZMBaseZombie, DT_ZM_BaseZombie )
    SendPropInt( SENDINFO( m_iSelectorIndex ) ),
    SendPropFloat( SENDINFO( m_flHealthRatio ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMBaseZombie )
END_DATADESC()


CZMBaseZombie::CZMBaseZombie()
{
    // We have to increment our population in derived classes since we don't know our class yet.
    g_pZombies->AddToTail( this );

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


    BaseClass::Spawn();
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
		m_hPhysicsEnt = NULL;

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
    case TASK_ZOMBIE_DIE :
        // Go to ragdoll
        KillMe();
        TaskComplete();
        break;

    case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ :
    {
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

void CZMBaseZombie::Command( const Vector& pos )
{
    m_vecLastPosition = pos;
    //SetCommandGoal( pos );

    //GetNavigator()->SetGoalTolerance( 128.0f );

    // This allows rally points to work without having to fuck around with the base class.
    if ( GetState() <= NPC_STATE_NONE )
    {
        SetState( NPC_STATE_ALERT );
    }
    
    SetSchedule( SCHED_FORCED_GO );
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