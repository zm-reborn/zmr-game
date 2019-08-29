#pragma once

#include "zmr/npcs/zmr_zombiebase.h"

#include "npcr_path_cost.h"
#include "zmr_zombie_swat.h"


static ConVar zm_sv_debug_swat_goto( "zm_sv_debug_swat_goto", "0" );


class GotoSwatObjSched : public NPCR::CSchedule<CZMBaseZombie>, public CZMSwatInt
{
private:
    SwatObjSched* m_pSwatSched;
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;
    CountdownTimer m_ExpireTimer;
    Vector m_vecStartSwatPos;
    Vector m_vecCurSwatPos;
    bool m_bCheckDirection;
    bool m_bCheckForEnemies; // Scan for enemies that may be closer than the swatting object
    float m_flStartExpireTimer;
    CountdownTimer m_EnemyScanTimer;
public:
    GotoSwatObjSched()
    {
        m_pSwatSched = new SwatObjSched();
        m_bCheckDirection = false;
        m_bCheckForEnemies = false;
        m_flStartExpireTimer = 0.0f;
    }
    ~GotoSwatObjSched()
    {
        delete m_pSwatSched;
    }


    void SetStartExpireTime( float time ) { m_flStartExpireTimer = time; }

    bool DoCheckDirection() const { return m_bCheckDirection; }
    void SetCheckDirection( bool state ) { m_bCheckDirection = state; }

    bool DoCheckForEnemies() const { return m_bCheckForEnemies; }
    void SetCheckForEnemies( bool state ) { m_bCheckForEnemies = state; }


    virtual const char* GetName() const OVERRIDE { return "ZombieGotoSwatObj"; }


    virtual void OnStart() OVERRIDE
    {
        m_ExpireTimer.Invalidate();
        m_EnemyScanTimer.Invalidate();


        CZMBaseZombie* pOuter = GetOuter();
        CBaseEntity* pSwat = pOuter->GetSwatObject();

        m_PathCost = *pOuter->GetPathCost();
        m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );


        if ( !pSwat )
        {
            End( "No object to swat!" );
            return;
        }

        if ( CheckObject( pSwat ) )
        {
            m_pSwatSched->SetBreakObject( DoBreakObject() );
            Intercept( m_pSwatSched, "Close enough to swat!" );
            
            if ( !IsIntercepted() )
            {
                End( "Failed to start swatting schedule!" );
                return;
            }
        }


        m_Path.SetGoalTolerance( 0.0f ); // Check goal manually.

        if ( !ComputeNewPath( pSwat->WorldSpaceCenter() ) )
            return;


        m_vecStartSwatPos = pSwat->WorldSpaceCenter();


        if ( m_flStartExpireTimer > 0.0f )
            m_ExpireTimer.Start( m_flStartExpireTimer );
    }

    virtual void OnContinue() OVERRIDE
    {
        End( "Swatting finished!" );
    }

    virtual void OnUpdate() OVERRIDE
    {
        if ( m_ExpireTimer.HasStarted() && m_ExpireTimer.IsElapsed() )
        {
            End( "Couldn't reach swattable object in time!" );
            return;
        }


        CZMBaseZombie* pOuter = GetOuter();

        CBaseEntity* pSwat = pOuter->GetSwatObject();
        if ( !pSwat )
        {
            End( "We have nothing to swat!" );
            return;
        }


        const Vector vecCurPos = pSwat->WorldSpaceCenter();

        // Scan for any enemies that are closer than the swatting object.
        if (DoCheckForEnemies() &&
            (!m_EnemyScanTimer.HasStarted() || m_EnemyScanTimer.IsElapsed()) )
        {
            CBaseEntity* pCurEnemy = pOuter->GetEnemy();
            CBaseEntity* pEnt = pOuter->GetSenses()->GetClosestEntity();

            if ( !pEnt )
                pEnt = pCurEnemy;

            if ( pEnt && pOuter->IsEnemy( pEnt ) )
            {
                auto mypos = pOuter->WorldSpaceCenter();

                float flCurDist = mypos.DistTo( vecCurPos );
                float flNewDist = mypos.DistTo( pEnt->WorldSpaceCenter() );

                
                if (pOuter->HasConditionsForClawAttack( pEnt ) // We can attack the idiot, do it!
                ||  flNewDist < (flCurDist*0.9f) ) // We have to be reasonably closer to the potential enemy.
                {
                    if ( !pCurEnemy || pEnt != pCurEnemy )
                        pOuter->AcquireEnemy( pEnt );

                    End( "Enemy is closer to us than the swatting object!" );
                    return;
                }
            }

            m_EnemyScanTimer.Start( 0.5f );
        }

        // We've moved way too far from the start.
        if ( m_vecStartSwatPos.DistToSqr( vecCurPos ) > (256.0f*256.0f) )
        {
            End( "Swatting object moved too far!" );
            return;
        }
        // Moved a bit, compute new path.
        else if ( m_vecCurSwatPos.DistToSqr( vecCurPos ) > (32.0f*32.0f) )
        {
            if ( !ComputeNewPath( vecCurPos ) )
                return;
        }


        if ( pOuter->CanMove() )
        {
            m_Path.Update( pOuter );
        }
            

        if ( !m_Path.IsValid() )
        {
            End( "Invalid path to swatting object!" );
            return;
        }

            
        if ( CheckObject( pSwat ) )
        {
            m_pSwatSched->SetBreakObject( DoBreakObject() );
            Intercept( m_pSwatSched, "Close enough to swat!" );

            if ( !IsIntercepted() )
            {
                End( "Failed to start swatting schedule!" );
            }

            return;
        }
    }

    virtual void OnChase( CBaseEntity* pEnemy ) OVERRIDE
    {
        TryEnd( "We're chasing an enemy, can't swat anymore!" );
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    virtual void OnCommanded( ZombieCommandType_t com ) OVERRIDE
    {
        TryEnd( "We were commanded to do something else!" );
    }

    bool CheckObject( CBaseEntity* pSwat )
    {
        if ( IsCloseEnough( pSwat ) )
        {
            // See if we can actually see and reach the object.

            // If it takes us longer than this to try to hit the object, stop.
            if ( !m_ExpireTimer.HasStarted() )
                m_ExpireTimer.Start( 3.0f );



            CZMBaseZombie* pOuter = GetOuter();
            const Vector vecSwatPos = pSwat->WorldSpaceCenter();

            // Ignore z axis but clamp to our size
            Vector vecMyPos = pOuter->GetAbsOrigin();
            vecMyPos.z = clamp( vecSwatPos.z, vecMyPos.z + 2.0f, vecMyPos.z + pOuter->CollisionProp()->OBBMaxs().z );

            Vector dir = vecSwatPos - vecMyPos;
            dir.NormalizeInPlace();

            // Only do a relatively small trace, so it doesn't look like we are telepathic.
            const Vector vecEnd = vecMyPos + dir * pOuter->CollisionProp()->OBBMaxs().x * 3.0f;

            trace_t tr;
            UTIL_TraceLine( vecMyPos, vecEnd, MASK_NPCSOLID & ~(CONTENTS_MONSTERCLIP), pOuter, COLLISION_GROUP_NONE, &tr );


            if ( zm_sv_debug_swat_goto.GetBool() )
            {
                bool bDidHit = tr.DidHit();
                NDebugOverlay::Line( vecMyPos, vecEnd, (!bDidHit) ? 255 : 0, bDidHit ? 255 : 0, 0, true, 0.1f );
            }

            // Didn't hit anything, check if our end is close enough to trace end pos.
            if ( tr.fraction == 1.0f )
            {
                return vecSwatPos.DistToSqr( tr.endpos ) < (16.0f*16.0f);
            }

            return tr.m_pEnt == pSwat;
        }

        return false;
    }

    bool IsCloseEnough( CBaseEntity* pSwat )
    {
        float radius = pSwat->BoundingRadius() + GetOuter()->GetClawAttackRange();
        return GetOuter()->GetAbsOrigin().AsVector2D().DistToSqr( pSwat->GetAbsOrigin().AsVector2D() ) < (radius*radius);
    }

    bool ComputeNewPath( const Vector& vecEnd )
    {
        const Vector vecStart = GetOuter()->GetAbsOrigin();

        CNavArea* start = GetOuter()->GetLastKnownArea();
        CNavArea* goal = TheNavMesh->GetNearestNavArea( vecEnd, true, 10000.0f, true );

            
        m_Path.Compute( vecStart, vecEnd, start, goal, m_PathCost );
        if ( !m_Path.IsValid() )
        {
            End( "Couldn't compute path to swat object!" );
            return false;
        }


        CBaseEntity* pEnemy = GetOuter()->GetEnemy();
        const NPCR::NavLink_t* pGoal = m_Path.FirstLink();

        // If we're for some reason going the OPPOSITE direction, stop right here. Criminal scum.
        if ( DoCheckDirection() && pGoal && pEnemy )
        {
            const NPCR::NavLink_t* pNext = m_Path.NextLink( pGoal );
            if ( pNext )
                pGoal = pNext;

            if ( pGoal )
            {
                if ( (pGoal->pos - vecStart).Normalized().Dot( (pEnemy->GetAbsOrigin() - vecStart).Normalized() ) < 0.0f )
                {
                    End( "Object path direction differs" );
                    return false;
                }
            }
        }


        m_vecCurSwatPos = vecEnd;

        m_PathCost.SetStartPos( vecStart, start );
        m_PathCost.SetGoalPos( vecEnd, goal );

        return true;
    }
};
