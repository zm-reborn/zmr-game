#include "cbase.h"
#include "vprof.h"

#include "zmr_player_shared.h"
#ifdef GAME_DLL
#include "zmr/npcs/zmr_zombiebase.h"
#endif
#include "zmr_softcollisions.h"



#ifdef GAME_DLL
extern ConVar zm_sv_zombiesoftcollisions;

ConVar zm_sv_softcollisions_zombie_size( "zm_sv_softcollisions_zombie_size", "24", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_softcollisions_zombie_force( "zm_sv_softcollisions_zombie_force", "700", FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_softcollisions_zombie_debug( "zm_sv_softcollisions_zombie_debug", "0" );
#endif


extern ConVar zm_sv_playercollision;

ConVar zm_sv_softcollisions_player_size( "zm_sv_softcollisions_player_size", "24", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_force( "zm_sv_softcollisions_player_force", "950", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_air_multiplier( "zm_sv_softcollisions_player_air_multiplier", "0.4", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED );
ConVar zm_sv_softcollisions_player_debug( "zm_sv_softcollisions_player_debug", "0", FCVAR_REPLICATED );


#ifdef GAME_DLL
void CZMSoftCollisions::FrameUpdatePostEntityThink()
#else
void CZMSoftCollisions::Update( float frametime )
#endif
{
    if ( zm_sv_playercollision.GetInt() == 1 )
    {
        PerformPlayerSoftCollisions();
    }

    ClearPlayerCollisions();


#ifdef GAME_DLL
    if ( zm_sv_zombiesoftcollisions.GetBool() )
    {
        PerformZombieSoftCollisions();
    }

    ClearZombieCollisions();
#endif
}

#ifdef GAME_DLL
void CZMSoftCollisions::PerformZombieSoftCollisions()
{
    VPROF_BUDGET( "CZMSoftCollisions::PerformZombieSoftCollisions", "NPCR" );



    int counter = 0;


    const float frametime = gpGlobals->frametime;
    const float flMaxDist = zm_sv_softcollisions_zombie_size.GetFloat();

    FOR_EACH_VEC( m_vZombieCollisions, i )
    {
        CZMBaseZombie* pOrigin = static_cast<CZMBaseZombie*>( m_vZombieCollisions[i].pOrigin );
        CZMBaseZombie* pOther = static_cast<CZMBaseZombie*>( m_vZombieCollisions[i].pOther );

        if ( !pOrigin->IsMoving() && pOther->IsMoving() )
            continue;


        Vector dir = pOrigin->GetLocalOrigin() - pOther->GetLocalOrigin();
        dir.z = 0.0f;


        Vector vel = pOrigin->GetMotor()->GetVelocity();


        const float flGroundSpd = pOrigin->m_flGroundSpeed;

        const float groundspeedmult = RemapValClamped( flGroundSpd, 100.0f, 200.0f, 1.0f, 2.0f );

        const float flMinPush = 20.0f;


        // Invert it so we push more the closer we are.
        float dist = dir.NormalizeInPlace();
        dist = MAX( 0.0f, flMaxDist - dist );
        if ( dist == 0.0f )
            continue;

        float force = RemapValClamped( dist, 0.0f, flMaxDist, 1.0f, 5.0f );
        force *= zm_sv_softcollisions_zombie_force.GetFloat() * groundspeedmult;


        

        if ( vel.Dot( dir ) < 0.0f || vel.Length2D() < flMinPush )
        {
            vel += dir * force * frametime;
            pOrigin->GetMotor()->SetVelocity( vel );

            ++counter;
        }
    }


    if ( zm_sv_softcollisions_zombie_debug.GetBool() && counter > 0 )
    {
        DevMsg( "Performed soft collisions %i times!\n", counter );
    }
}
#endif

void CZMSoftCollisions::PerformPlayerSoftCollisions()
{
    VPROF_BUDGET( "CZMSoftCollisions::PerformPlayerSoftCollisions", VPROF_BUDGETGROUP_CLIENT_SIM );



    int counter = 0;


    const float frametime = gpGlobals->frametime;
    const float flMaxDist = zm_sv_softcollisions_player_size.GetFloat();

    FOR_EACH_VEC( m_vPlayerCollisions, i )
    {
        CZMPlayer* pOrigin = static_cast<CZMPlayer*>( m_vPlayerCollisions[i].pOrigin );
        CZMPlayer* pOther = static_cast<CZMPlayer*>( m_vPlayerCollisions[i].pOther );


        Vector vel = pOrigin->GetLocalVelocity();
        //Vector vel2 = pOther->GetLocalVelocity();

        // Don't apply soft collisions if we (the origin) are not moving.
        // Assume the other will perform the pushing to us.
        //if ( vel == vec3_origin && vel2 != vec3_origin )
        //    continue;


        Vector dir = pOrigin->GetLocalOrigin() - pOther->GetLocalOrigin();
        dir.z = 0.0f;




        // Invert it so we push more the closer we are.
        float dist = dir.NormalizeInPlace();
        dist = MAX( 0.0f, flMaxDist - dist );
        if ( dist == 0.0f )
            continue;

        float force = RemapValClamped( dist, 0.0f, flMaxDist, 1.0f, 5.0f );
        force *= zm_sv_softcollisions_player_force.GetFloat();
        

        if ( !(pOrigin->GetFlags() & FL_ONGROUND) )
        {
            force *= zm_sv_softcollisions_player_air_multiplier.GetFloat();
        }


        const float flMinPush = 100.0f;

        if ( vel.Dot( dir ) < 0.0f || vel.Length2D() < flMinPush )
        {
            vel += dir * force * frametime;
            pOrigin->SetLocalVelocity( vel );

            ++counter;
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

#ifdef GAME_DLL
void CZMSoftCollisions::OnZombieCollide( CBaseEntity* pOrigin, CBaseEntity* pOther )
{
    // Do we already have this collision?
    FOR_EACH_VEC( m_vZombieCollisions, i )
    {
        if ( m_vZombieCollisions[i].pOrigin == pOrigin && m_vZombieCollisions[i].pOther == pOther )
            return;
    }


    m_vZombieCollisions.AddToTail( { pOrigin, pOther } );
}
#endif

void CZMSoftCollisions::OnPlayerCollide( CBaseEntity* pOrigin, CBaseEntity* pOther )
{
    // Do we already have this collision?
    FOR_EACH_VEC( m_vPlayerCollisions, i )
    {
        if ( m_vPlayerCollisions[i].pOrigin == pOrigin && m_vPlayerCollisions[i].pOther == pOther )
            return;
    }


    m_vPlayerCollisions.AddToTail( { pOrigin, pOther } );
}


void CZMSoftCollisions::ClearPlayerCollisions()
{
    m_vPlayerCollisions.RemoveAll();
}

#ifdef GAME_DLL
void CZMSoftCollisions::ClearZombieCollisions()
{
    m_vZombieCollisions.RemoveAll();
}
#endif


static CZMSoftCollisions g_ZMSoftCollisions;

CZMSoftCollisions* GetZMSoftCollisions()
{
    return &g_ZMSoftCollisions;
}
