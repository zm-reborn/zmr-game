#include "cbase.h"
#include "vprof.h"
#include "in_buttons.h"

#include "zmr_player_shared.h"
#ifdef GAME_DLL
#include "npcs/zmr_zombiebase.h"
#endif
#include "zmr_softcollisions.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#ifdef GAME_DLL
extern ConVar zm_sv_zombiesoftcollisions;

ConVar zm_sv_softcollisions_zombie_size( "zm_sv_softcollisions_zombie_size", "24", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_softcollisions_zombie_force( "zm_sv_softcollisions_zombie_force", "700", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_softcollisions_zombie_debug( "zm_sv_softcollisions_zombie_debug", "0" );
#endif


extern ConVar zm_sv_playercollision;

ConVar zm_sv_softcollisions_player_size( "zm_sv_softcollisions_player_size", "32", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_force( "zm_sv_softcollisions_player_force", "60", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_air_multiplier( "zm_sv_softcollisions_player_air_multiplier", "0.1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_debug( "zm_sv_softcollisions_player_debug", "0", FCVAR_REPLICATED );


//
#ifdef GAME_DLL
// Perform zombie collisions
CZMBaseSoftCol::SoftColRes_t CZMZombieSoftCol::PerformCollision()
{
    const float flMaxDist = zm_sv_softcollisions_zombie_size.GetFloat();



    CZMBaseZombie* pOrigin = static_cast<CZMBaseZombie*>( hOrigin.Get() );
    CZMBaseZombie* pOther = static_cast<CZMBaseZombie*>( hOther.Get() );
    if ( !pOrigin || !pOther || !pOrigin->IsAlive() || !pOther->IsAlive() )
    {
        return CZMBaseSoftCol::COLRES_INVALID;
    }


    Vector dir = pOrigin->GetLocalOrigin() - pOther->GetLocalOrigin();
    if ( dir.x == 0.0f && dir.y == 0.0f ) // Always push somewhere.
    {
        dir.x = vecLastDir.x;
        dir.y = vecLastDir.y;
    }
    dir.z = 0.0f;

    // Invert it so we push more the closer we are.
    float dist = dir.NormalizeInPlace();
    dist = MAX( 0.0f, flMaxDist - dist );
    if ( dist == 0.0f ) // We're no longer in pushing distance, clear us from the list.
    {
        return CZMBaseSoftCol::COLRES_INVALID;
    }


    // Assume the zombie who's not moving is there for a reason.
    if ( !pOrigin->IsMoving() && pOther->IsMoving() )
    {
        return CZMBaseSoftCol::COLRES_NONE;
    }


    Vector vel = pOrigin->GetMotor()->GetVelocity();


    const float flGroundSpd = pOrigin->m_flGroundSpeed;

    const float groundspeedmult = RemapValClamped( flGroundSpd, 100.0f, 200.0f, 1.0f, 2.0f );

    const float flMinPush = 20.0f;


    float force = RemapValClamped( dist, 0.0f, flMaxDist, 1.0f, 5.0f );
    force *= zm_sv_softcollisions_zombie_force.GetFloat() * groundspeedmult;


        

    if ( vel.Dot( dir ) < 0.0f || vel.Length2D() < flMinPush )
    {
        vel += dir * force * gpGlobals->frametime;
        pOrigin->GetMotor()->SetVelocity( vel );

        vecLastDir = dir.AsVector2D();

        return CZMBaseSoftCol::COLRES_APPLIED;
    }



    return CZMBaseSoftCol::COLRES_NONE;
}

//
void CZMSoftCollisions::FrameUpdatePostEntityThink()
{
    if ( zm_sv_zombiesoftcollisions.GetBool() )
    {
        PerformZombieSoftCollisions();
    }
    else
    {
        ClearZombieCollisions();
    }
}

void CZMSoftCollisions::PerformZombieSoftCollisions()
{
    VPROF_BUDGET( "CZMSoftCollisions::PerformZombieSoftCollisions", "NPCR" );



    int counter = 0;

    FOR_EACH_VEC( m_vZombieCollisions, i )
    {
        CZMBaseSoftCol::SoftColRes_t ret = m_vZombieCollisions[i].PerformCollision();

        switch ( ret )
        {
        case CZMBaseSoftCol::COLRES_APPLIED :
            ++counter;
        case CZMBaseSoftCol::COLRES_INVALID :
            m_vZombieCollisions.Remove( i );
            --i;
        }
    }


    if ( zm_sv_softcollisions_zombie_debug.GetBool() && counter > 0 )
    {
        DevMsg( "Performed soft collisions %i times!\n", counter );
    }
}

void CZMSoftCollisions::OnZombieCollide( CBaseEntity* pOrigin, CBaseEntity* pOther )
{
    // Do we already have this collision?
    FOR_EACH_VEC( m_vZombieCollisions, i )
    {
        if ( m_vZombieCollisions[i].Equals( pOrigin, pOther ) )
            return;
    }


    CZMZombieSoftCol col( pOrigin, pOther );

    m_vZombieCollisions.AddToTail( col );
}

void CZMSoftCollisions::ClearZombieCollisions()
{
    m_vZombieCollisions.RemoveAll();
}
#endif

#ifdef CLIENT_DLL
void CZMSoftCollisions::OnPlayerCollide( CBaseEntity* pOrigin, CBaseEntity* pOther )
{
    // Do we already have this collision?
    FOR_EACH_VEC( m_vPlayerCollisions, i )
    {
        if ( m_vPlayerCollisions[i].Equals( pOrigin, pOther ) )
            return;
    }


    CZMPlayerSoftCol col( pOrigin, pOther );

    m_vPlayerCollisions.AddToTail( col );
}

void CZMSoftCollisions::PerformPlayerSoftCollisions( CUserCmd* pCmd )
{
    if ( zm_sv_playercollision.GetInt() != 1 )
    {
        ClearPlayerCollisions();
        return;
    }

    VPROF_BUDGET( "CZMSoftCollisions::PerformPlayerSoftCollisions", VPROF_BUDGETGROUP_CLIENT_SIM );



    int counter = 0;


    FOR_EACH_VEC( m_vPlayerCollisions, i )
    {
        CZMBaseSoftCol::SoftColRes_t ret = m_vPlayerCollisions[i].PerformCollision( pCmd );

        switch ( ret )
        {
        case CZMBaseSoftCol::COLRES_APPLIED :
            ++counter;
            break;
        case CZMBaseSoftCol::COLRES_INVALID :
        default :
            m_vPlayerCollisions.Remove( i );
            --i;
        }
    }


    if ( zm_sv_softcollisions_player_debug.GetBool() && counter > 0 )
    {
        const bool bIsServer =
#ifdef CLIENT_DLL
            false;
#else
            true;
#endif
        DevMsg( "Performed soft collisions %i times! (%s)\n", counter, bIsServer ? "server" : "client" );
    }
}

void CZMSoftCollisions::ClearPlayerCollisions()
{
    m_vPlayerCollisions.RemoveAll();
}
#endif

// Perform player collisions
CZMBaseSoftCol::SoftColRes_t CZMPlayerSoftCol::PerformCollision( CUserCmd* pCmd )
{
    const float flMaxDist = zm_sv_softcollisions_player_size.GetFloat();



    CZMPlayer* pOrigin = static_cast<CZMPlayer*>( hOrigin.Get() );
    CZMPlayer* pOther = static_cast<CZMPlayer*>( hOther.Get() );
    if (
        !pOrigin || !pOther || !pOrigin->IsAlive() || !pOther->IsAlive()
#ifdef CLIENT_DLL
        || pOther->IsDormant()
#endif
    )
    {
        return CZMBaseSoftCol::COLRES_INVALID;
    }


    Vector vel = pOrigin->GetLocalVelocity();
    Vector vel2 = pOther->GetLocalVelocity();


    Vector dir = pOrigin->GetLocalOrigin() - pOther->GetLocalOrigin();
    if ( dir.x == 0.0f && dir.y == 0.0f ) // Always push somewhere. Players may be hugging a corner.
    {
        dir.x = vecLastDir.x;
        dir.y = vecLastDir.y;
    }
    dir.z = 0.0f;


    // Invert it so we push more the closer we are.
    float dist = dir.NormalizeInPlace();
    dist = MAX( 0.0f, flMaxDist - dist );
    if ( dist == 0.0f ) // We're no longer in pushing distance, clear us from the list.
    {
        return CZMBaseSoftCol::COLRES_INVALID;
    }


    // Don't apply soft collisions if we (the origin) are not moving.
    // Assume the other will perform the pushing to us.
    bool bMovingOrigin = vel != vec3_origin;
    bool bMovingOther = vel2 != vec3_origin;

    if ( !bMovingOrigin && bMovingOther )
    {
        return CZMBaseSoftCol::COLRES_NONE;
    }

    if ( !bMovingOrigin && !bMovingOther )
    {
        // Both not moving, don't do anything if the origin didn't move at any point.
        if ( !m_bOriginMovedLast )
            return CZMBaseSoftCol::COLRES_NONE;
    }

    float force = RemapValClamped( dist, 0.0f, flMaxDist, 0.1f, 5.0f );
    force *= zm_sv_softcollisions_player_force.GetFloat();
        

    if ( !(pOrigin->GetFlags() & FL_ONGROUND) )
    {
        force *= zm_sv_softcollisions_player_air_multiplier.GetFloat();
    }


    const float flMinPush = 100.0f;

    if ( vel.Dot( dir ) < 0.0f || vel.Length2D() < flMinPush )
    {
        vel = dir * force;

        Vector fwd, right;
        AngleVectors( pCmd->viewangles, &fwd, &right, nullptr );

        pCmd->forwardmove += fwd.Dot( vel );
        pCmd->sidemove += right.Dot( vel );


        vecLastDir = dir.AsVector2D();
        m_bOriginMovedLast = bMovingOrigin;


        return CZMBaseSoftCol::COLRES_APPLIED;
    }


    return CZMBaseSoftCol::COLRES_NONE;
}
//



static CZMSoftCollisions g_ZMSoftCollisions;

CZMSoftCollisions* GetZMSoftCollisions()
{
    return &g_ZMSoftCollisions;
}
