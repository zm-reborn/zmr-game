#include "cbase.h"
#include "NPCevent.h"


#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"

#include "zmr_zombiebase.h"



//ZombieList_t g_Zombies;



IMPLEMENT_SERVERCLASS_ST( CZMBaseZombie, DT_ZM_BaseZombie )
    SendPropInt( SENDINFO( m_iSelectorIndex ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMBaseZombie )
END_DATADESC()


CZMBaseZombie::CZMBaseZombie()
{
    // We have to increment our population in derived classes since we don't know our class yet.


    g_pZombies->AddToTail( this );

    m_LagTrack = new CUtlFixedLinkedList<LagRecordNPC>();


    m_iSelectorIndex = 0;


}

CZMBaseZombie::~CZMBaseZombie()
{
    g_pZombies->FindAndRemove( this );

    m_LagTrack->Purge();
    delete m_LagTrack;



    // It's safe to remove pop count here.
    CZMRules* pRules = ZMRules();

    // Will assert when changing maps.
    //Assert( pRules );

    if ( !pRules ) return;


    pRules->SetZombiePop( pRules->GetZombiePop() - GetPopCost() );
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

    return BaseClass::HandleAnimEvent( pEvent );
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

void CZMBaseZombie::Spawn()
{
    m_fIsHeadless = false;
    m_fIsTorso = false;

    BaseClass::Spawn();
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

bool CZMBaseZombie::Swat( CBaseEntity* pTarget )
{
    if ( !pTarget ) return false;

    if ( !CanSwatPhysicsObjects() ) return false;


    m_hPhysicsEnt = pTarget;
    
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