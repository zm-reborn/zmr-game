#pragma once

#include "npcs/zmr_zombiebase.h"

#include "zmr_zombie_swat_goto.h"



extern ConVar zm_sv_swat_scan_target_maxdist;
extern ConVar zm_sv_swat_scan_def_maxdist;

#define ZOMBIE_PHYSICS_SEARCH_DEPTH     32


class ScanSwatObjSched : public NPCR::CSchedule<CZMBaseZombie>, public CZMSwatInt
{
private:
    GotoSwatObjSched* m_pGotoSwatSched;
public:
    ScanSwatObjSched()
    {
        m_pGotoSwatSched = new GotoSwatObjSched;
        // Only go towards the enemy.
        m_pGotoSwatSched->SetCheckDirection( true );
        // Stop the swatting if we have enemies closer.
        m_pGotoSwatSched->SetCheckForEnemies( true );
    }
    ~ScanSwatObjSched()
    {
        delete m_pGotoSwatSched;
    }

    virtual const char* GetName() const OVERRIDE { return "ZombieScanSwatObj"; }


    virtual void OnStart() OVERRIDE
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

    virtual void OnContinue() OVERRIDE
    {
        End( "Swatting finished!" );
    }

    // We should never reach here.
    virtual void OnUpdate() OVERRIDE
    {
        Assert( 0 );
        End( "Schedule error occured in swatting scan!" );
    }

    CBaseEntity* GetSwatObject()
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

    CBaseEntity* FindNearestSwatObject( const Vector& vecDirToGoal, const Vector& vecTarget, float flFurthestDist )
    {
        CBaseEntity* pList[ZOMBIE_PHYSICS_SEARCH_DEPTH];
        CBaseEntity* pNearest = nullptr;
        float flNearestDist = 1000.0f;
        float flDist;
        IPhysicsObject* pPhysObj;
        int i;
        trace_t tr;

        const unsigned int tracemask = MASK_SOLID & ~(CONTENTS_MONSTER);
        CTraceFilterWorldOnly filter;


        CZMBaseZombie* pOuter = GetOuter();
        const Vector vecMyPos = pOuter->GetAbsOrigin();

        CBaseEntity* pEnemy = pOuter->GetEnemy();


        // Find objects within a box between us and the enemy
        Vector vecDelta( flFurthestDist / 2.0f, flFurthestDist / 2.0f, pOuter->GetMotor()->GetHullHeight() / 2.0f );

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


                return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
            }
        };

        CZombieSwatEntitiesEnum swatEnum( pList, ARRAYSIZE( pList ) );


        int count = UTIL_EntitiesInBox( vecBoxOrigin - vecDelta, vecBoxOrigin + vecDelta, &swatEnum );


        // We can only reach this low to break the object.
        Vector vecLowest = vecMyPos + Vector( 0.0f, 0.0f, pOuter->GetAttackLowest() );

        for ( i = 0; i < count; i++ )
        {
            pPhysObj = pList[ i ]->VPhysicsGetObject();

            Assert( pPhysObj );


            Vector center = pList[ i ]->WorldSpaceCenter();
            Vector dir = center - vecMyPos;
            dir.z = 0.0f;
            flDist = dir.NormalizeInPlace();

            if ( flDist >= flNearestDist )
                continue;
        

            // This object is closer... but is it between the player and the zombie?
            if ( DotProduct( vecDirToGoal, dir ) < 0.9f )
                continue;

            //if ( flDist >= UTIL_DistApprox2D( center, vecTarget ) )
            //    continue;

            // don't swat things where the highest point is under my knees
            // NOTE: This is a rough test; a more exact test is going to occur below
            if ( (center.z + pList[i]->BoundingRadius()) < vecLowest.z )
                continue;

            // don't swat things that are over my head.
            if ( center.z > pOuter->EyePosition().z )
                continue;


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


            vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
        
            Vector objMins, objMaxs;
            physcollision->CollideGetAABB( &objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

            if ( objMaxs.z < vecLowest.z )
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

        if ( IsDebugging() )
            NDebugOverlay::Box( vecBoxOrigin, -vecDelta, vecDelta, pNearest ? 255 : 0, (!pNearest) ? 255 : 0, 0, 0, 0.5f );

        return pNearest;
    }
};
