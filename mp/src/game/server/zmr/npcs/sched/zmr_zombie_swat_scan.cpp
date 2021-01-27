#include "cbase.h"

#include "zmr_zombie_swat_scan.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define ZOMBIE_PHYSICS_SEARCH_DEPTH     32



CZombieScanSwatObjSchedule::CZombieScanSwatObjSchedule()
{
    m_pGotoSwatSched = new CZombieGotoSwatObjSchedule;
    // Only go towards the enemy.
    m_pGotoSwatSched->SetCheckDirection( true );
    // Stop the swatting if we have enemies closer.
    m_pGotoSwatSched->SetCheckForEnemies( true );
}

CZombieScanSwatObjSchedule::~CZombieScanSwatObjSchedule()
{
    delete m_pGotoSwatSched;
}

void CZombieScanSwatObjSchedule::OnStart()
{
    CBaseEntity* pSwat = GetSwatObject();
    if ( !pSwat )
    {
        End( "Couldn't find swattable object!" );
        return;
    }

    GetOuter()->SetSwatObject( pSwat );

    m_pGotoSwatSched->SetBreakObject( DoBreakObject() );
    Intercept( m_pGotoSwatSched, "Found a swattable object! Go to it!" );

    if ( !IsIntercepted() )
    {
        End( "Failed to start go to swat schedule!" );
    }
}

void CZombieScanSwatObjSchedule::OnContinue()
{
    End( "Swatting finished!" );
}

// We should never reach here.
void CZombieScanSwatObjSchedule::OnUpdate()
{
    Assert( 0 );
    End( "Schedule error occured in swatting scan!" );
}

CBaseEntity* CZombieScanSwatObjSchedule::GetSwatObject()
{
    CZMBaseZombie* pOuter = GetOuter();

    const Vector vecMyPos = pOuter->GetAbsOrigin();


    Vector vecDirToGoal;
    Vector vecDirToObject;
    Vector vecTarget;
            

    CBaseEntity* pEnemy = pOuter->GetEnemy();
    if ( pEnemy )
    {
        vecTarget = pEnemy->GetAbsOrigin();
    }
    else
    {
        vecTarget = vecMyPos + UTIL_YawToVector( pOuter->GetAbsAngles().y ) * 200.0f;
    }


    vecDirToGoal = vecTarget - vecMyPos;

    float maxdist = zm_sv_swat_scan_target_maxdist.GetFloat();
    if ( pEnemy && vecDirToGoal.IsLengthGreaterThan( maxdist ) )
    {
        return nullptr;
    }


    vecDirToGoal.z = 0;
    vecDirToGoal.NormalizeInPlace();

    float flDist = pEnemy ? MAX( 32.0f, vecMyPos.DistTo( vecTarget ) ) : zm_sv_swat_scan_def_maxdist.GetFloat();

    return FindNearestSwatObject( vecDirToGoal, vecTarget, flDist );
}

CBaseEntity* CZombieScanSwatObjSchedule::FindNearestSwatObject( const Vector& vecDirToGoal, const Vector& vecTarget, float flFurthestDist )
{
    CBaseEntity* pList[ZOMBIE_PHYSICS_SEARCH_DEPTH];

    CZMBaseZombie* pOuter = GetOuter();
    const Vector vecMyPos = pOuter->GetAbsOrigin();

    CBaseEntity* pEnemy = pOuter->GetEnemy();


    // Find objects within a box between us and the enemy
    Vector vecDelta( flFurthestDist * 0.5f, flFurthestDist * 0.5f, pOuter->GetMotor()->GetHullHeight() * 0.5f );

    Vector vecBoxOrigin = vecMyPos + vecDirToGoal * vecDelta;
    vecBoxOrigin.z += vecDelta.z;
            

    class CZombieSwatEntitiesEnum : public CFlaggedEntitiesEnum
    {
    public:
        CZombieSwatEntitiesEnum( CBaseEntity** pList, int listMax ) : CFlaggedEntitiesEnum( pList, listMax, 0 )
        {
        }

        virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
        {
            // Easier debugging
            CBaseEntity* pEnt = EntityFromEntityHandle( pHandleEntity );
            if ( !pEnt )
                return ITERATION_CONTINUE;
            if ( !pEnt->IsSolid() )
                return ITERATION_CONTINUE;
            if ( pEnt->GetCollisionGroup() == COLLISION_GROUP_WEAPON ) // Don't bother trying to swat ammo/weapons.
                return ITERATION_CONTINUE;

            if ( !CZMBaseZombie::CanSwatObject( pEnt ) )
                return ITERATION_CONTINUE;

            // Don't swat server ragdolls!
            if ( FClassnameIs( pEnt, "physics_prop_ragdoll" ) )
                return ITERATION_CONTINUE;
            
            if ( FClassnameIs( pEnt, "prop_ragdoll" ) )
                return ITERATION_CONTINUE;


            return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
        }
    };

    CZombieSwatEntitiesEnum swatEnum( pList, ARRAYSIZE( pList ) );


    int count = UTIL_EntitiesInBox( vecBoxOrigin - vecDelta, vecBoxOrigin + vecDelta, &swatEnum );


    // We can only reach this low to break the object.
    Vector vecLowest = vecMyPos + Vector( 0.0f, 0.0f, pOuter->GetAttackLowest() );

    const unsigned int tracemask = MASK_SOLID & ~(CONTENTS_MONSTER);
    CTraceFilterWorldOnly filter;
    CBaseEntity* pNearest = nullptr;
    float flNearestDist = FLT_MAX;
    trace_t tr;
    Vector dir, center, closestPoint;
    float flDist;

    for ( int i = 0; i < count; i++ )
    {
        center = pList[ i ]->WorldSpaceCenter();

        // Check if the prop is COMPLETELY opposite.
        // There are cases where the prop is large enough
        // To have the center be a bit behind us.
        dir = center - vecMyPos;
        dir.z = 0.0f;
        dir.NormalizeInPlace();

        if ( vecDirToGoal.Dot( dir ) < -0.6f )
            continue;


        // Don't swat things where the highest point is under my knees
        // NOTE: This is a rough test; a more exact test is going to occur below
        if ( (center.z + pList[i]->BoundingRadius()) < vecLowest.z )
            continue;
            
        // Don't swat things that are over my head.
        if ( center.z > pOuter->EyePosition().z )
            continue;


        auto* pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
        
        Vector objMins, objMaxs;
        physcollision->CollideGetAABB( &objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

        // Highest point too low.
        if ( objMaxs.z < vecLowest.z )
            continue;


        // Check the closest point direction.
        // This ensures we get the closest possible prop to swat.
        CalcClosestPointOnAABB( objMins, objMaxs, vecMyPos, closestPoint );
        dir = closestPoint - vecMyPos;
        dir.z = 0.0f;
        flDist = dir.NormalizeInPlace();

        // It's possible to be inside the bounding box.
        if ( flDist > 0.0f )
        {
            // Must be head-on
            if ( vecDirToGoal.Dot( dir ) < 0.9f )
                continue;
        }


        // We must see the prop.
        UTIL_TraceLine( center, pOuter->EyePosition(), tracemask, &filter, &tr );
        if ( tr.fraction != 1.0f )
            continue;

        // The prop must have LOS to the target or there's no point in swatting it towards them.
        if ( pEnemy )
        {
            UTIL_TraceLine( center, vecTarget, tracemask, &filter, &tr );
            if ( tr.fraction != 1.0f )
                continue;
        }


        pNearest = pList[i];
        flNearestDist = flDist;
    }

    if ( IsDebugging() )
        NDebugOverlay::Box( vecBoxOrigin, -vecDelta, vecDelta, pNearest ? 255 : 0, (!pNearest) ? 255 : 0, 0, 0, 0.5f );

    return pNearest;
}