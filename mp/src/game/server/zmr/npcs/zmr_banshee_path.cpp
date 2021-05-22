#include "cbase.h"

#include "zmr_banshee.h"
#include "zmr_banshee_path.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_debug_banshee_path_jump( "zm_sv_debug_banshee_path_jump", "0" );

bool CZMBansheeFollowPath::Compute( const Vector& vecStart, const Vector& vecGoal, CNavArea* pStartArea, CNavArea* pGoalArea, const NPCR::CBasePathCost& cost )
{
    Invalidate();

    if ( !CanPotentiallyJumpTo( vecStart, vecGoal ) )
    {
        return BaseClass::Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
    }


    float height = FindJumpHeight( vecStart, vecGoal );

    if ( height < 0.0f )
    {
        return BaseClass::Compute( vecStart, vecGoal, pStartArea, pGoalArea, cost );
    }


    m_flHeight = height;
    BuildSimplePath( vecStart, vecGoal, true );

    m_Links[0].navTravel = NPCR::TRAVEL_NAVJUMP;

    return true;
}

void CZMBansheeFollowPath::Invalidate()
{
    m_flHeight = -1.0f;
    BaseClass::Invalidate();
}

NPCR::PathRes_t CZMBansheeFollowPath::CheckGoal( NPCR::CBaseNPC* pNPC ) const
{
    // If we're about to jump, don't do any goal checks yet.
    if ( m_JumpStatus == NPCR::JUMP_BEGIN && GetCurLink() && GetCurLink()->navTravel == NPCR::TRAVEL_NAVJUMP )
    {
        return NPCR::PATH_OK;
    }


    return BaseClass::CheckGoal( pNPC );
}

void CZMBansheeFollowPath::OnNewGoal( NPCR::CBaseNPC* pNPC )
{
    m_JumpStatus = NPCR::JUMP_BEGIN;
}

void CZMBansheeFollowPath::UpdateMove( NPCR::CBaseNPC* pNPC )
{
    if ( GetCurLink() && GetCurLink()->navTravel == NPCR::TRAVEL_NAVJUMP && pNPC->GetMotor()->IsOnGround() )
    {
        if ( pNPC->GetMotor()->IsFacing( GetGoalLink()->pos, 10.0f ) )
        {
            m_JumpStatus = NPCR::JUMP_IN_AIR;
            pNPC->GetMotor()->NavJump( GetGoalLink()->pos, m_flHeight );
            m_flHeight = -1.0f;
        }
        else
        {
            pNPC->GetMotor()->FaceTowards( GetGoalLink()->pos );
        }

        return;
    }

    BaseClass::UpdateMove( pNPC );
}

bool CZMBansheeFollowPath::CanPotentiallyJumpTo( const Vector& vecStart, const Vector& vecGoal ) const
{
    Vector dir = vecGoal - vecStart;
    dir.z = 0.0f;
    float height = vecGoal.z - vecStart.z;
    float dist = dir.NormalizeInPlace();

    if ( fabsf( height ) < 24.0f && dist < 128.0f )
    {
        return false;
    }

    // Too far away
    if ( dist > CZMBanshee::GetMaxNavJumpDist() )
    {
        return false;
    }

    // The jump up is too high.
    if ( height > CZMBanshee::GetMaxNavJumpHeight() )
    {
        return false;
    }


    // Check LOS, if we do, we don't need to!
    Vector up( 0.0f, 0.0f, 2.0f );
    trace_t tr;
    CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NPC );
    UTIL_TraceLine(
        vecStart + up,
        vecGoal + up,
        MASK_NPCSOLID, &filter, &tr );

    if ( !tr.m_pEnt )
        return false;

    return true;
}

float CZMBansheeFollowPath::FindJumpHeight( const Vector& vecStart, const Vector& vecGoal ) const
{
    const bool bDebugging = zm_sv_debug_banshee_path_jump.GetBool();

    if ( bDebugging )
    {
        DevMsg( "Attempting to find jump height...\n" );
    }


    float ret_height = -1.0f;

    

    
    // Trace until we don't hit obstacles and return the height.

    const float flMaxClearance = CZMBanshee::GetBansheeHullHeight();
    float clearance = 0.0f;
    float flIncrementZ = 16.0f;


    const float flMaxJump = CZMBanshee::GetMaxNavJumpHeight();

    CTraceFilterNoNPCsOrPlayer filter( nullptr, COLLISION_GROUP_NPC );
    trace_t tr;

    float end_z = vecStart.z + flMaxJump;
    Vector start = vecStart;
    Vector end = vecGoal;
    if ( end.z < start.z ) // Going downwards, we can safely ignore height.
        end.z = start.z;
    else if ( end.z > start.z ) // Going up, start at the end height.
        start.z = end.z;

    start.z += 1.0f; // Start a bit off the ground
    end.z += 1.0f;

    bool bCheckClearance = false;
    float jump_z = vecStart.z;
    
    while ( start.z < end_z )
    {
        UTIL_TraceLine( start, end, MASK_NPCSOLID, &filter, &tr );

        if ( tr.startsolid )
            break;


        bool bHit = tr.fraction < 0.9f;
        if ( bDebugging )
        {
            NDebugOverlay::Line( start, end, bHit ? 255 : 0, (!bHit) ? 255 : 0, 0, true, 1.0f );
        }


        if ( !bHit )
        {
            if ( bCheckClearance )
            {
                clearance += flIncrementZ;
            }
            else
            {
                // First time we have clear path.
                jump_z = start.z;

                flIncrementZ *= 2.0f;
                end_z = start.z + flMaxClearance + flIncrementZ * 2.0f;

                // Start to check for clearance
                float add = CZMBanshee::GetBansheeHullHeight() - flIncrementZ;
                if ( end.z <= start.z )
                    end.z += add;
                start.z += add;
            }

            bCheckClearance = true;
        }
        

        if ( (bHit && bCheckClearance) || clearance >= flMaxClearance )
        {
            // Add height based on how much we have room.
            float height_add = clamp( clearance / flMaxClearance, 0.0f, 1.0f ) * CZMBanshee::GetBansheeHullHeight();
            ret_height = jump_z - vecStart.z + height_add;
            break;
        }


        start.z += flIncrementZ;

        if ( end.z <= start.z )
            end.z += flIncrementZ;
    }


    if ( ret_height == -1.0f )
        return -1.0f;


    // Finally, make sure we can actually get to the end by tracing to it.
    start = end;
    start.z = vecStart.z + ret_height;
    end.z = vecGoal.z + 1.0f;

    UTIL_TraceLine( start, end, MASK_NPCSOLID & ~(CONTENTS_MONSTER), &filter, &tr );
    bool bHit = tr.fraction != 1.0f;

    if ( bDebugging )
    {
        NDebugOverlay::Line( start, end, bHit ? 255 : 0, (!bHit) ? 255 : 0, 0, true, 1.0f );
    }

    if ( bHit )
    {
        return -1.0f;
    }

    return ret_height;
}
