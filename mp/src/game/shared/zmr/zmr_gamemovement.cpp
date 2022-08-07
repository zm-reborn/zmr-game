#include "cbase.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "convar.h"
#include "collisionutils.h"

#ifndef CLIENT_DLL
#include "player.h"

#include "env_player_surface_trigger.h"
#else
#include "c_baseplayer.h"
#endif


#include "zmr_gamerules.h"

#ifndef CLIENT_DLL
#include "zmr_player.h"
#else
#include "c_zmr_player.h"
#endif

#include "zmr_softcollisions.h"
#include "zmr_gamemovement.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_maxbunnyhopspeed( "zm_sv_maxbunnyhopspeed", "300", FCVAR_REPLICATED | FCVAR_NOTIFY );


#define LADDER_AUTOMOUNT_DIST       32.0f
#define LADDER_AUTOMOUNT_DOT        0.4f
#define LADDER_DISMOUNT_DOT         0.8f
#define LADDER_DISMOUNT_MAX_DIST    100.0f
#define LADDER_MAX_DIST             64.0f


extern bool g_bMovementOptimizations;



extern ConVar zm_sv_playercollision;


// Custom move filter to implement soft collisions.
class CPlayerMoveFilter : public CTraceFilterSimple
{
public:
    CPlayerMoveFilter( CBasePlayer* pMe, int collisionGroup ) : CTraceFilterSimple( pMe, collisionGroup )
    {
        m_pMe = pMe;
        m_pMe->CollisionProp()->WorldSpaceAABB( &m_vecMins, &m_vecMaxs );
        m_vecMins += 1.0f;
        m_vecMaxs -= 1.0f;
    }

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE
    {
        if ( pHandleEntity == GetPassEntity() )
            return false;


        
        CBaseEntity* pEnt = EntityFromEntityHandle( pHandleEntity );
        if ( pEnt && pEnt->IsPlayer() )
        {
#ifdef CLIENT_DLL
            C_ZMPlayer* pLocal = C_ZMPlayer::GetLocalPlayer();
#endif
            if (zm_sv_playercollision.GetInt() == 1
#ifdef CLIENT_DLL
                // Only for local player. Prediction is nice.
            &&  (m_pMe == pLocal || pEnt == pLocal || (pLocal && pLocal->GetObserverTarget() == m_pMe))
#endif
                )
            {
#ifdef CLIENT_DLL
                // Pass our hit player to soft collisions.
                GetZMSoftCollisions()->OnPlayerCollide( m_pMe, pEnt );
#endif
            }
            // If we have collisions on and they're inside one another, ignore collision.
            else if ( zm_sv_playercollision.GetInt() >= 2 )
            {
                Vector otmins, otmaxs;
                pEnt->CollisionProp()->WorldSpaceAABB( &otmins, &otmaxs );
                return !IsBoxIntersectingBox( m_vecMins, m_vecMaxs, otmins, otmaxs );
            }


            return false;
        }



        return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
    }
private:
    CBasePlayer* m_pMe;
    Vector m_vecMins;
    Vector m_vecMaxs;
};

void CZMGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
    // Do our own bbox trace to account for soft collisions and stuckness


    VPROF( "CGameMovement::TracePlayerBBox" );


    CBasePlayer* pMe = static_cast<CBasePlayer*>( EntityFromEntityHandle( mv->m_nPlayerHandle.Get() ) );
    CPlayerMoveFilter filter( pMe, collisionGroup );
    Ray_t ray;
    ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );


    enginetrace->TraceRay( ray, fMask, &filter, &pm );
}

CBaseHandle CZMGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
    // Do our own bbox trace to account for soft collisions and stuckness

    CBasePlayer* pMe = static_cast<CBasePlayer*>( EntityFromEntityHandle( mv->m_nPlayerHandle.Get() ) );
    CPlayerMoveFilter filter( pMe, collisionGroup );
    Ray_t ray;
    ray.Init( pos, pos, GetPlayerMins(), GetPlayerMaxs() );


    enginetrace->TraceRay( ray, PlayerSolidMask(), &filter, &pm );

    if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt )
    {
        return pm.m_pEnt->GetRefEHandle();
    }
    else
    {	
        return INVALID_EHANDLE_INDEX;
    }
}

void CZMGameMovement::TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm )
{
    // Do our own bbox trace to account for soft collisions and stuckness


    VPROF( "CGameMovement::TryTouchGround" );


    CBasePlayer* pMe = static_cast<CBasePlayer*>( EntityFromEntityHandle( mv->m_nPlayerHandle.Get() ) );
    CPlayerMoveFilter filter( pMe, collisionGroup );
    Ray_t ray;
    ray.Init( start, end, mins, maxs );


    enginetrace->TraceRay( ray, fMask, &filter, &pm );
}

void CZMGameMovement::PlayerMove( void )
{
    VPROF( "CGameMovement::PlayerMove" );

    CheckParameters();
    
    // clear output applied velocity
    mv->m_outWishVel.Init();
    mv->m_outJumpVel.Init();

    MoveHelper( )->ResetTouchList();                    // Assume we don't touch anything

    ReduceTimers();

    AngleVectors (mv->m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp );  // Determine movement angles

    // Always try and unstick us unless we are using a couple of the movement modes
    if ( player->GetMoveType() != MOVETYPE_NOCLIP && 
         player->GetMoveType() != MOVETYPE_NONE && 		 
         player->GetMoveType() != MOVETYPE_ISOMETRIC && 
         player->GetMoveType() != MOVETYPE_OBSERVER && 
         !player->pl.deadflag )
    {
        if ( CheckInterval( STUCK ) )
        {
            if ( CheckStuck() )
            {
                // Can't move, we're stuck
                return;  
            }
        }
    }

    // Now that we are "unstuck", see where we are (player->GetWaterLevel() and type, player->GetGroundEntity()).
    if ( player->GetMoveType() != MOVETYPE_WALK ||
        mv->m_bGameCodeMovedPlayer || 
        !g_bMovementOptimizations  )
    {
        CategorizePosition();
    }
    else
    {
        if ( mv->m_vecVelocity.z > 250.0f )
        {
            SetGroundEntity( nullptr );
        }
    }

    // Store off the starting water level
    m_nOldWaterLevel = player->GetWaterLevel();

    // If we are not on ground, store off how fast we are moving down
    if ( !player->GetGroundEntity() )
    {
        player->m_Local.m_flFallVelocity = -mv->m_vecVelocity[ 2 ];
    }


    // player->m_pSurfaceData had to replaced by GetSurfaceData... really valve...
    player->UpdateStepSound( player->GetSurfaceData(), mv->GetAbsOrigin(), mv->m_vecVelocity );

    UpdateDuckJumpEyeOffset();
    Duck();

    // Don't run ladder code if dead on on a train
    if ( !player->pl.deadflag && !(player->GetFlags() & FL_ONTRAIN) )
    {
        // If was not on a ladder now, but was on one before, 
        //  get off of the ladder
        
        // TODO: this causes lots of weirdness.
        //bool bCheckLadder = CheckInterval( LADDER );
        //if ( bCheckLadder || player->GetMoveType() == MOVETYPE_LADDER )
        {
            if ( !LadderMove() && 
                ( player->GetMoveType() == MOVETYPE_LADDER ) )
            {
                // Clear ladder stuff unless player is dead or riding a train
                // It will be reset immediately again next frame if necessary
                player->SetMoveType( MOVETYPE_WALK );
                player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
            }
        }
    }

    // Handle movement modes.
    switch (player->GetMoveType())
    {
        case MOVETYPE_NONE:
            break;

        case MOVETYPE_NOCLIP:
            // ZMRTODO: Use our own move values.
            if ( ToZMPlayer( player )->IsZM() )
            {
                FullZMMove();
            }
            else
            {
                FullNoClipMove( sv_noclipspeed.GetFloat(), sv_noclipaccelerate.GetFloat() );
            }

            break;

        case MOVETYPE_FLY:
        case MOVETYPE_FLYGRAVITY:
            FullTossMove();
            break;

        case MOVETYPE_LADDER:
            FullLadderMove();
            break;

        case MOVETYPE_WALK:
            FullWalkMove();
            break;

        case MOVETYPE_ISOMETRIC:
            //IsometricMove();
            // Could also try:  FullTossMove();
            FullWalkMove();
            break;
            
        case MOVETYPE_OBSERVER:
            FullObserverMove(); // clips against world&players
            break;

        default:
            DevMsg( 1, "Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", player->GetMoveType(), player->IsServer());
            break;
    }


    //
    // Check for view clearance.
    // Eg. if player is crouched under something, don't let the view clip through objects.
    //
    auto* pPlayer = GetZMPlayer();
    if ( pPlayer->IsHuman() && pPlayer->IsAlive() )
    {
        const float flClearance = 11.0f;
        const float flHalfHullSize = 8.0f;

        Vector viewoff = GetPlayerViewOffset( pPlayer->IsDucked() );
        Vector vecStartPos = pPlayer->GetAbsOrigin() + viewoff;
        
        Vector vecEndPos = vecStartPos;
        vecEndPos.z += flClearance;

        vecStartPos.z -= 2.0f; // Lower the start a bit for startsolid check.


        CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_PLAYER );
        trace_t tr;

        Vector mins(-flHalfHullSize, -flHalfHullSize, 0 );
        Vector maxs( flHalfHullSize,  flHalfHullSize, 0 );

        UTIL_TraceHull( vecStartPos, vecEndPos, mins, maxs, PlayerSolidMask(), &filter, &tr );

        if ( tr.fraction != 1.0f && !tr.startsolid )
        {
            float offset = vecEndPos.z - tr.endpos.z;

            viewoff.z -= offset;
            pPlayer->SetViewOffset( viewoff );
        }
        else if ( !pPlayer->IsDucking() ) // If in the process of ducking/unducking, don't change.
        {
            pPlayer->SetViewOffset( viewoff );
        }
    }
}

Vector CZMGameMovement::GetPlayerMins() const
{
    if ( GetZMPlayer()->IsZM() )
    {
        return VEC_ZM_HULL_MIN;
    }

    return CGameMovement::GetPlayerMins();
}

Vector CZMGameMovement::GetPlayerMins( bool ducked ) const
{
    if ( GetZMPlayer()->IsZM() )
    {
        return VEC_ZM_HULL_MIN;
    }

    return CGameMovement::GetPlayerMins( ducked );
}

Vector CZMGameMovement::GetPlayerMaxs() const
{
    if ( GetZMPlayer()->IsZM() )
    {
        return VEC_ZM_HULL_MAX;
    }

    return CGameMovement::GetPlayerMaxs();
}

Vector CZMGameMovement::GetPlayerMaxs( bool ducked ) const
{
    if ( GetZMPlayer()->IsZM() )
    {
        return VEC_ZM_HULL_MAX;
    }

    return CGameMovement::GetPlayerMaxs( ducked );
}

Vector CZMGameMovement::GetPlayerViewOffset( bool ducked ) const
{
    if ( GetZMPlayer()->IsZM() )
    {
        return VEC_ZM_VIEW;
    }

    return CGameMovement::GetPlayerViewOffset( ducked );
}

unsigned int CZMGameMovement::PlayerSolidMask( bool brushOnly )
{
    if ( GetZMPlayer()->IsZM() )
    {
        return CONTENTS_TEAM1;
    }

    return CGameMovement::PlayerSolidMask();
}

void CZMGameMovement::CategorizePosition()
{
    Vector point;
    trace_t pm;

    // Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
    player->m_surfaceFriction = 1.0f;

    // if the player hull point one unit down is solid, the player
    // is on ground
    
    // see if standing on something solid	

    // Doing this before we move may introduce a potential latency in water detection, but
    // doing it after can get us stuck on the bottom in water if the amount we move up
    // is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
    // this several times per frame, so we really need to avoid sticking to the bottom of
    // water on each call, and the converse case will correct itself if called twice.
    CheckWater();

    // observers don't have a ground entity
    if ( player->IsObserver() )
        return;

    // ZMRCHANGE: No need to update ground entity for ZM.
    if ( ToZMPlayer( player )->IsZM() )
        return;

    float flOffset = 2.0f;

    point[0] = mv->GetAbsOrigin()[0];
    point[1] = mv->GetAbsOrigin()[1];
    point[2] = mv->GetAbsOrigin()[2] - flOffset;

    Vector bumpOrigin;
    bumpOrigin = mv->GetAbsOrigin();

    // Shooting up really fast.  Definitely not on ground.
    // On ladder moving up, so not on ground either
    // NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY 140.0f

    float zvel = mv->m_vecVelocity[2];
    bool bMovingUp = zvel > 0.0f;
    bool bMovingUpRapidly = zvel > NON_JUMP_VELOCITY;
    float flGroundEntityVelZ = 0.0f;
    if ( bMovingUpRapidly )
    {
        // Tracker 73219, 75878:  ywb 8/2/07
        // After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
        //  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
        // We need to account for standing on a moving ground object in that case in order to determine if we really 
        //  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
        //  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
        CBaseEntity *ground = player->GetGroundEntity();
        if ( ground )
        {
            flGroundEntityVelZ = ground->GetAbsVelocity().z;
            bMovingUpRapidly = ( zvel - flGroundEntityVelZ ) > NON_JUMP_VELOCITY;
        }
    }

    // Was on ground, but now suddenly am not
    if ( bMovingUpRapidly || 
        ( bMovingUp && player->GetMoveType() == MOVETYPE_LADDER ) )   
    {
        SetGroundEntity( nullptr );
    }
    else
    {
        // Try and move down.
        TryTouchGround( bumpOrigin, point, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
        
        // Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
        if ( !pm.m_pEnt || pm.plane.normal[2] < 0.7 )
        {
            // Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
            TryTouchGroundInQuadrants( bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

            if ( !pm.m_pEnt || pm.plane.normal[2] < 0.7 )
            {
                SetGroundEntity( nullptr );
                // probably want to add a check for a +z velocity too!
                if ( ( mv->m_vecVelocity.z > 0.0f ) && 
                    ( player->GetMoveType() != MOVETYPE_NOCLIP ) )
                {
                    player->m_surfaceFriction = 0.25f;
                }
            }
            else
            {
                SetGroundEntity( &pm );
            }
        }
        else
        {
            SetGroundEntity( &pm );  // Otherwise, point to index of ent under us.
        }

#ifndef CLIENT_DLL
        
        //Adrian: vehicle code handles for us.
        if ( player->IsInAVehicle() == false )
        {
            // If our gamematerial has changed, tell any player surface triggers that are watching
            IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
            surfacedata_t *pSurfaceProp = physprops->GetSurfaceData( pm.surface.surfaceProps );
            char cCurrGameMaterial = pSurfaceProp->game.material;
            if ( !player->GetGroundEntity() )
            {
                cCurrGameMaterial = 0;
            }

            // Changed?
            if ( player->m_chPreviousTextureType != cCurrGameMaterial )
            {
                CEnvPlayerSurfaceTrigger::SetPlayerSurface( player, cCurrGameMaterial );
            }

            player->m_chPreviousTextureType = cCurrGameMaterial;
        }
#endif
    }
}

static ConVar zm_sv_accelerate_fix( "zm_sv_accelerate_fix", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Is ground-strafing limited? Ie. players can't go faster by spamming strafe keys/wall-boosting." );

void CZMGameMovement::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
    int i;
    float addspeed, accelspeed, currentspeed;

    // This gets overridden because some games (CSPort) want to allow dead (observer) players
    // to be able to move around.
    if ( !CanAccelerate() )
        return;


    // See if we are changing direction a bit

    // Stops wall-strafing/pre-strafing.
    if ( zm_sv_accelerate_fix.GetBool() )
    {
        currentspeed = sqrt( DotProduct( mv->m_vecVelocity, mv->m_vecVelocity ) );
    }
    else
    {
        currentspeed = mv->m_vecVelocity.Dot( wishdir );
    }

    // Reduce wishspeed by the amount of veer.
    addspeed = wishspeed - currentspeed;

    // If not going to add any speed, done.
    if ( addspeed <= 0 )
        return;

    // Determine amount of accleration.
    accelspeed = accel * gpGlobals->frametime * wishspeed * player->m_surfaceFriction;

    // Cap at addspeed
    if ( accelspeed > addspeed )
        accelspeed = addspeed;
    
    // Adjust velocity.
    for ( i = 0; i < 3; i++ )
    {
        mv->m_vecVelocity[i] += accelspeed * wishdir[i];	
    }
}

void CZMGameMovement::FullZMMove()
{
    // Client sends its wanted max speed, acceleration, etc. through forward/side move and userinfo convars.
    // See C_ZMPlayer::CreateMove
    // Also see CZMPlayer::UpdatePlayerZMVars

    Vector wishvel;
    Vector forward, right, up;
    Vector wishdir;
    float wishspeed;
    float maxspeed;
    float accel;
    float decel;


    GetZMPlayer()->GetZMMovementVars( maxspeed, accel, decel );

    AngleVectors( mv->m_vecViewAngles, &forward, &right, &up );  // Determine movement angles
    

    
    // Copy movement amounts
    // The real wanted movement speed is scaled down, so here we scale it back up.
    float fmove = mv->m_flForwardMove * 100.0f;
    float smove = mv->m_flSideMove * 100.0f;
    float umove = mv->m_flUpMove * 100.0f;
    
    if ( !(mv->m_nButtons & IN_ZM_OBSERVERMODE) )
    {
        forward[2] = 0;
        right[2] = 0;
    }

    VectorNormalize( forward );
    VectorNormalize( right );


    for ( int i = 0; i < 3; i++ )
        wishvel[i] = forward[i]*fmove + right[i]*smove;
    wishvel[2] += umove;


    VectorCopy( wishvel, wishdir );   // Determine maginitude of speed of move
    wishspeed = VectorNormalize( wishdir );

    if ( wishspeed > maxspeed )
    {
        VectorScale( wishvel, maxspeed/wishspeed, wishvel );
        wishspeed = maxspeed;
    }
    


    Accelerate( wishdir, wishspeed, accel );

    float spd = VectorLength( mv->m_vecVelocity );
    if ( spd < 1.0f )
    {
        mv->m_vecVelocity.Init();
        return;
    }


        
    // Bleed off some speed, but if we have less than the bleed
    //  threshhold, bleed the theshold amount.
    float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;
    
    float friction = decel;

    // Add the amount to the drop amount.
    float drop = control * friction * gpGlobals->frametime;

    // scale the velocity
    float newspeed = spd - drop;
    if ( newspeed < 0 )
        newspeed = 0;

    // Determine proportion of old speed we are using.
    newspeed /= spd;
    VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );



    // Do the actual moving


    Vector out;
    VectorMA( mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, out );

    // Yes, we do need to trace for the ZM. This is for ZM clips.
    trace_t pm;
    TracePlayerBBox( mv->GetAbsOrigin(), out, PlayerSolidMask(), COLLISION_GROUP_NONE, pm );

    if ( pm.fraction == 1.0f || pm.startsolid )
    {
        mv->SetAbsOrigin( out );
    }
    else
    {
        TryPlayerMove( &out, &pm );
    }
}

void CZMGameMovement::PlayerRoughLandingEffects( float fvol )
{
    if ( GetZMPlayer()->IsZM() )
        return;

    CGameMovement::PlayerRoughLandingEffects( fvol );
}

void CZMGameMovement::Duck()
{
    if ( GetZMPlayer()->IsZM() )
        return;

    CGameMovement::Duck();
}

void CZMGameMovement::FinishDuck()
{
    // This was causing problems.
    //if ( player->GetFlags() & FL_DUCKING )
    //	return;

    player->AddFlag( FL_DUCKING );
    player->m_Local.m_bDucked = true;
    player->m_Local.m_bDucking = false;

    player->SetViewOffset( GetPlayerViewOffset( true ) );

    // HACKHACK - Fudge for collision bug - no time to fix this properly
    if ( player->GetGroundEntity() )
    {
        for ( int i = 0; i < 3; i++ )
        {
            Vector org = mv->GetAbsOrigin();
            org[ i ]-= ( VEC_DUCK_HULL_MIN_SCALED( player )[i] - VEC_HULL_MIN_SCALED( player )[i] );
            mv->SetAbsOrigin( org );
        }
    }
    else
    {
        Vector hullSizeNormal = VEC_HULL_MAX_SCALED( player ) - VEC_HULL_MIN_SCALED( player );
        Vector hullSizeCrouch = VEC_DUCK_HULL_MAX_SCALED( player ) - VEC_DUCK_HULL_MIN_SCALED( player );
        Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );
        Vector out;
        VectorAdd( mv->GetAbsOrigin(), viewDelta, out );
        mv->SetAbsOrigin( out );

#ifdef CLIENT_DLL
#ifdef STAGING_ONLY
        if ( debug_latch_reset_onduck.GetBool() )
        {
            player->ResetLatched();
        }
#else
        player->ResetLatched();
#endif
#endif // CLIENT_DLL
    }

    // See if we are stuck?
    FixPlayerCrouchStuck( true );

    // Recategorize position since ducking can change origin
    CategorizePosition();
}

bool CZMGameMovement::CheckJumpButton( void )
{
    if (player->pl.deadflag)
    {
        mv->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
        return false;
    }

    // See if we are waterjumping.  If so, decrement count and return.
    if (player->m_flWaterJumpTime)
    {
        player->m_flWaterJumpTime -= gpGlobals->frametime;
        if (player->m_flWaterJumpTime < 0)
            player->m_flWaterJumpTime = 0;
        
        return false;
    }

    // If we are in the water most of the way...
    if ( player->GetWaterLevel() >= 2 )
    {	
        // swimming, not jumping
        SetGroundEntity( nullptr );

        if(player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
            mv->m_vecVelocity[2] = 100;
        else if (player->GetWaterType() == CONTENTS_SLIME)
            mv->m_vecVelocity[2] = 80;
        
        // play swiming sound
        if ( player->m_flSwimSoundTime <= 0 )
        {
            // Don't play sound again for 1 second
            player->m_flSwimSoundTime = 1000;
            PlaySwimSound();
        }

        return false;
    }

    // No more effect
    if ( !player->GetGroundEntity() )
    {
        mv->m_nOldButtons |= IN_JUMP;
        return false;		// in air, so no effect
    }

    // Don't allow jumping when the player is in a stasis field.
#ifndef HL2_EPISODIC
    if ( player->m_Local.m_bSlowMovement )
        return false;
#endif

    if ( mv->m_nOldButtons & IN_JUMP )
        return false;		// don't pogo stick

    // Cannot jump will in the unduck transition.
    if ( player->m_Local.m_bDucking && (  player->GetFlags() & FL_DUCKING ) )
        return false;

    // Still updating the eye position.
    if ( player->m_Local.m_flDuckJumpTime > 0.0f )
        return false;


    // In the air now.
    SetGroundEntity( nullptr );
    
    player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );
    
    //MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );
    // ZMRCHANGE
    GetZMPlayer()->DoAnimationEvent( PLAYERANIMEVENT_JUMP );

    float flGroundFactor = 1.0f;
    if (player->m_pSurfaceData)
    {
        flGroundFactor = player->m_pSurfaceData->game.jumpFactor; 
    }


    //
    // IMPORTANT:
    // Player hull size was changed to stop the view jerking
    // when crouching/uncrouching in air. Crouch jumping gets affected by this change.
    // You cannot reach as high. Here we increase the normal jump height by 7
    // to maintain the same height.
    // 21.333... is the normal intended jump height.
    //
    Assert( GetCurrentGravity() == 600.0f );
    float flMul = sqrtf( 2.0f * GetCurrentGravity() * (21.333333f + 7) );


    // Acclerate upward
    // If we are ducking...
    float startz = mv->m_vecVelocity[2];
    if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) )
    {
        // d = 0.5 * g * t^2		- distance traveled with linear accel
        // t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
        // v = g * t				- velocity at the end (just invert it to jump up that high)
        // v = g * sqrt(2.0 * 45 / g )
        // v^2 = g * g * 2.0 * 45 / g
        // v = sqrt( g * 2.0 * 45 )
        mv->m_vecVelocity[2] = flGroundFactor * flMul;  // 2 * gravity * height
    }
    else
    {
        mv->m_vecVelocity[2] += flGroundFactor * flMul;  // 2 * gravity * height
    }



    FinishGravity();

    mv->m_outJumpVel.z += mv->m_vecVelocity[2] - startz;
    mv->m_outStepHeight += 0.15f;

    OnJump(mv->m_outJumpVel.z);



    // Restrict bunnyhopping speed

    float maxspd = zm_sv_maxbunnyhopspeed.GetFloat();
    if ( maxspd >= 0.0f )
    {
        float length = mv->m_vecVelocity.Length2D();

        // We have to take into account the base velocity (push triggers, etc.)
        // Otherwise the player could throttle the speed given by the push and fuck things up.
        maxspd = MAX( player->GetBaseVelocity().Length2D(), maxspd );

        if ( length > 0.1f && length > maxspd )
        {
            float mult = maxspd / length;
            mv->m_vecVelocity[0] *= mult;
            mv->m_vecVelocity[1] *= mult;
        }
    }


    // Flag that we jumped.
    mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
    return true;
}

float CZMGameMovement::ClimbSpeed() const
{
    return 200.0f;
}

LadderMoveRet_t CZMGameMovement::LadderMove_Brush()
{
    trace_t pm;
    bool onFloor;
    Vector floor;
    Vector wishdir;
    Vector end;

    if ( player->GetMoveType() == MOVETYPE_NOCLIP )
        return LADDERMOVERET_NO_LADDER;


    // If I'm already moving on a ladder, use the previous ladder direction
    if ( player->GetMoveType() == MOVETYPE_LADDER )
    {
        wishdir = -player->m_vecLadderNormal;
    }
    else
    {
        // otherwise, use the direction player is attempting to move
        if ( mv->m_flForwardMove || mv->m_flSideMove )
        {
            for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
                wishdir[i] = m_vecForward[i]*mv->m_flForwardMove + m_vecRight[i]*mv->m_flSideMove;

            wishdir.NormalizeInPlace();
        }
        else
        {
            // Player is not attempting to move, no ladder behavior
            return LADDERMOVERET_NO_LADDER;
        }
    }

    // wishdir points toward the ladder if any exists
    end = mv->GetAbsOrigin() + wishdir * LadderDistance();

    TracePlayerBBox( mv->GetAbsOrigin(), end, LadderMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

    // no ladder in that direction, return
    if ( pm.fraction == 1.0f || !OnLadder( pm ) )
        return LADDERMOVERET_NO_LADDER;

    player->SetMoveType( MOVETYPE_LADDER );
    player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

    player->m_vecLadderNormal = pm.plane.normal;

    // On ladder, convert movement to be relative to the ladder

    floor = mv->GetAbsOrigin();
    floor[2] += GetPlayerMins()[2] - 1;

    if( enginetrace->GetPointContents( floor ) == CONTENTS_SOLID || player->GetGroundEntity() )
    {
        onFloor = true;
    }
    else
    {
        onFloor = false;
    }


    float climbSpeed = ClimbSpeed();

    float forwardSpeed = 0, rightSpeed = 0;
    if ( mv->m_nButtons & IN_BACK )
        forwardSpeed -= climbSpeed;
    
    if ( mv->m_nButtons & IN_FORWARD )
        forwardSpeed += climbSpeed;
    
    if ( mv->m_nButtons & IN_MOVELEFT )
        rightSpeed -= climbSpeed;
    
    if ( mv->m_nButtons & IN_MOVERIGHT )
        rightSpeed += climbSpeed;

    if ( mv->m_nButtons & IN_JUMP )
    {
        ExitLadder();

        mv->m_vecVelocity = pm.plane.normal * climbSpeed;

        return LADDERMOVERET_DISMOUNTED;
    }
    else if ( forwardSpeed != 0 || rightSpeed != 0 )
    {
        Vector velocity, perp, cross, lateral, tmp;

        //ALERT(at_console, "pev %.2f %.2f %.2f - ",
        //	pev->velocity.x, pev->velocity.y, pev->velocity.z);
        // Calculate player's intended velocity
        //Vector velocity = (forward * gpGlobals->v_forward) + (right * gpGlobals->v_right);
        velocity = m_vecForward * forwardSpeed;
        velocity += m_vecRight * rightSpeed;

        // Make sure player can't go faster than the climb speed.
        velocity = velocity.Normalized() * climbSpeed;

        // Perpendicular in the ladder plane
        tmp = vec3_origin;
        tmp[2] = 1;
        perp = tmp.Cross( pm.plane.normal );
        perp.NormalizeInPlace();

        // decompose velocity into ladder plane
        float normal = velocity.Dot( pm.plane.normal );

        // This is the velocity into the face of the ladder
        VectorScale( pm.plane.normal, normal, cross );

        // This is the player's additional velocity
        lateral = velocity - cross;

        // This turns the velocity into the face of the ladder into velocity that
        // is roughly vertically perpendicular to the face of the ladder.
        // NOTE: It IS possible to face up and move down or face down and move up
        // because the velocity is a sum of the directional velocity and the converted
        // velocity through the face of the ladder -- by design.
        tmp = pm.plane.normal.Cross( perp );


        VectorMA( lateral, -normal, tmp, mv->m_vecVelocity );

        if ( onFloor && normal > 0 )	// On ground moving away from the ladder
        {
            mv->m_vecVelocity += pm.plane.normal * climbSpeed;
        }
        //pev->velocity = lateral - (CrossProduct( trace.vecPlaneNormal, perp ) * normal);
    }
    else
    {
        mv->m_vecVelocity.Init();
    }

    return LADDERMOVERET_ONLADDER;
}

CFuncLadder* CZMGameMovement::FindLadder( Vector& ladderOrigin, const CFuncLadder* skipLadder )
{
    CFuncLadder* bestLadder = nullptr;
    float bestDist = FLT_MAX;
    Vector bestOrigin;

    bestOrigin.Init();

    float maxdistSqr = LADDER_MAX_DIST * LADDER_MAX_DIST;


    int c = CFuncLadder::GetLadderCount();
    for ( int i = 0 ; i < c; i++ )
    {
        auto* pLadder = CFuncLadder::GetLadder( i );

        if ( !pLadder->IsEnabled() )
            continue;

        if ( skipLadder && pLadder == skipLadder )
            continue;

        Vector topPosition;
        Vector bottomPosition;

        pLadder->GetTopPosition( topPosition );
        pLadder->GetBottomPosition( bottomPosition );

        Vector closest;
        CalcClosestPointOnLineSegment( mv->GetAbsOrigin(), bottomPosition, topPosition, closest, nullptr );

        float distSqr = ( closest - mv->GetAbsOrigin() ).LengthSqr();

        // Too far away
        if ( distSqr > maxdistSqr )
        {
            continue;
        }

        // Need to trace to see if it's clear
        trace_t tr;

        UTIL_TraceLine( mv->GetAbsOrigin(), closest, 
            MASK_PLAYERSOLID,
            player,
            COLLISION_GROUP_NONE,
            &tr );

        if ( tr.fraction != 1.0f &&
             tr.m_pEnt &&
             tr.m_pEnt != pLadder )
        {
            // Try a trace stepped up from the ground a bit, in case there's something at ground level blocking us.
            float sizez = GetPlayerMaxs().z - GetPlayerMins().z;

            UTIL_TraceLine( mv->GetAbsOrigin() + Vector( 0, 0, sizez * 0.5f ), closest, 
                MASK_PLAYERSOLID,
                player,
                COLLISION_GROUP_NONE,
                &tr );

            if ( tr.fraction != 1.0f &&
                 tr.m_pEnt &&
                 tr.m_pEnt != pLadder &&
                 !tr.m_pEnt->IsSolidFlagSet( FSOLID_TRIGGER ) )
            {
                continue;
            }
        }

        // See if this is the best one so far
        if ( distSqr < bestDist )
        {
            bestDist = distSqr;
            bestLadder = pLadder;
            bestOrigin = closest;
        }
    }


    ladderOrigin = bestOrigin;

    return bestLadder;

}

bool CZMGameMovement::CheckLadderMount( CFuncLadder* pLadder, Vector& vecBestPos )
{
    Assert( pLadder );

    Vector top, bottom;
    pLadder->GetTopPosition( top );
    pLadder->GetBottomPosition( bottom );


    int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );
    int buttonsPressed = buttonsChanged & mv->m_nButtons;
    
    bool bPressedUse = ( buttonsPressed & IN_USE ) ? true : false;

    bool bOnGround = GetZMPlayer()->GetGroundEntity() != nullptr;


    Vector dirToTop = top - mv->GetAbsOrigin();

    float distToTop = dirToTop.NormalizeInPlace();
    dirToTop.z = 0.0f;
    dirToTop.NormalizeInPlace();


    Vector dirToBottom = bottom - mv->GetAbsOrigin();
    float distToBottom = dirToBottom.NormalizeInPlace();
    dirToBottom.z = 0.0f;
    dirToBottom.NormalizeInPlace();


    bool bBottomCloser = distToTop > distToBottom;

    float flMoveTowardsLadderDot;
    float flLookTowardsLadderDot;

    {
        Vector wishdir;
        for ( int i = 0; i < 2; i++ )
            wishdir[i] = mv->m_flForwardMove * m_vecForward[i] + mv->m_flSideMove * m_vecRight[i];
        wishdir[2] = 0.0f;
        wishdir.NormalizeInPlace();

        flMoveTowardsLadderDot = bBottomCloser ? dirToBottom.Dot( wishdir ) : dirToTop.Dot( wishdir );


        Vector fwd = m_vecForward;
        fwd.z = 0.0f;
        fwd.NormalizeInPlace();

        flLookTowardsLadderDot = bBottomCloser ? dirToBottom.Dot( fwd ) : dirToTop.Dot( fwd );
    }


    bool bMovingTowardsLadder = flMoveTowardsLadderDot > 0.7f;


    if ( bOnGround && bMovingTowardsLadder )
    {
        //
        // We're on ground, check if they are near top/bottom points.
        //
        if ( distToTop < LADDER_AUTOMOUNT_DIST || distToBottom < LADDER_AUTOMOUNT_DIST )
        {
            Vector ladderNormal;


            if ( !bBottomCloser )
            {
                // Close to top
                ladderNormal = bottom - top;
            }
            else
            {
                ladderNormal = top - bottom;
            }

            VectorNormalize( ladderNormal );

            if ( ladderNormal.Dot( m_vecForward ) > 0.0f )
            {
                vecBestPos = bBottomCloser ? bottom : top;
                return true;
            }
        }
    }


    if ( bPressedUse || (!bOnGround && bMovingTowardsLadder) )
    {
        if ( flLookTowardsLadderDot > LADDER_AUTOMOUNT_DOT )
        {
            return true;
        }
    }

    return false;
}

LadderMoveRet_t CZMGameMovement::LadderMove_HL2()
{
    if ( player->GetMoveType() == MOVETYPE_NOCLIP )
    {
        ExitLadder();
        return LADDERMOVERET_NO_LADDER;
    }

    // If being forced to mount/dismount continue to act like we are on the ladder
    if ( IsForceMoveActive() && ContinueForcedMove() )
    {
        return LADDERMOVERET_ONLADDER;
    }

    CFuncLadder* pNewLadder = nullptr;
    Vector bestOrigin( 0, 0, 0 );

    auto* pLadder = GetLadder();

    // Something 1) deactivated the ladder...  or 2) something external applied
    //  a force to us.  In either case  make the player fall, etc.
    if ( (pLadder && !pLadder->IsEnabled()) || player->GetBaseVelocity().LengthSqr() > 1.0f )
    {
        ExitLadder();

        return LADDERMOVERET_NO_LADDER;
    }

    if ( !pLadder )
    {
        pNewLadder = FindLadder( bestOrigin, nullptr );
    }


    if ( !pLadder && pNewLadder )
    {
        if ( CheckLadderMount( pNewLadder, bestOrigin ) )
        {
            StartForcedMove( true, player->MaxSpeed(), bestOrigin, pNewLadder );

            return LADDERMOVERET_ONLADDER;
        }
    }


    pLadder = GetLadder();
    if ( !pLadder )
    {
        return LADDERMOVERET_NO_LADDER;
    }

    //
    // We're on ladder, do the moving and stuff.
    //
    Vector top;
    Vector bottom;

    pLadder->GetTopPosition( top );
    pLadder->GetBottomPosition( bottom );


    int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
    int buttonsPressed = buttonsChanged & mv->m_nButtons;


    float distTopSqr = ( top - mv->GetAbsOrigin() ).LengthSqr();
    float distBottomSqr = ( bottom - mv->GetAbsOrigin() ).LengthSqr();

    bool bPressedJump = buttonsPressed & IN_JUMP;
    bool bPressedUse = buttonsPressed & IN_USE;

    bool bNearMountPoints = distTopSqr < (LADDER_AUTOMOUNT_DIST*LADDER_AUTOMOUNT_DIST) || distBottomSqr < (LADDER_AUTOMOUNT_DIST*LADDER_AUTOMOUNT_DIST);
    
    //
    // Player wants to leave the ladder.
    //
    if ( bPressedJump || bPressedUse || bNearMountPoints )
    {
        bool forcedismount = bPressedJump || bPressedUse;

        if ( ExitLadderViaDismountNode( pLadder, forcedismount ) )
        {
            return LADDERMOVERET_DISMOUNTED;
        }

        if ( forcedismount )
        {
            ExitLadder();


            // Jump in view direction
            if ( bPressedJump )
            {
                Vector jumpDir = m_vecForward;

                // unless pressing backward or something like that
                if ( mv->m_flForwardMove < 0.0f )
                {
                    jumpDir = -jumpDir;
                }

                jumpDir.NormalizeInPlace();

                mv->m_vecVelocity = jumpDir * ClimbSpeed();

                // Tracker 13558:  Don't add any extra z velocity if facing downward at all
                if ( m_vecForward.z >= 0.0f )
                {
                    mv->m_vecVelocity.z = mv->m_vecVelocity.z + 50;
                }
            }

            return LADDERMOVERET_DISMOUNTED;
        }
    }


#define MAX_DIST_FROM_LADDER_PATH		4.0f

    // Check to see if we've mounted the ladder in a bogus spot and, if so, just fall off the ladder...
    float t;
    float distFromLadderSqr = CalcDistanceSqrToLine( mv->GetAbsOrigin(), bottom, top, &t );
    if ( distFromLadderSqr > (MAX_DIST_FROM_LADDER_PATH*MAX_DIST_FROM_LADDER_PATH) )
    {
        ExitLadder();
        return LADDERMOVERET_DISMOUNTED;
    }

    // We need to be clamped to the ladder or the player may be able to climb to heaven
    // if the dismount somehow fails.
    if ( t < 0.0f )
    {
        mv->SetAbsOrigin( bottom );
    }
    else if ( t > 1.0f )
    {
        mv->SetAbsOrigin( top );
    }


    // Make sure we are on the ladder
    player->SetMoveType( MOVETYPE_LADDER );
    player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
    
    player->SetGroundEntity( nullptr );


    float forwardSpeed = 0.0f;
    float rightSpeed = 0.0f;

    float speed = player->MaxSpeed();

    if ( mv->m_nButtons & IN_BACK )
    {
        forwardSpeed -= speed;
    }
    
    if ( mv->m_nButtons & IN_FORWARD )
    {
        forwardSpeed += speed;
    }
    
    if ( mv->m_nButtons & IN_MOVELEFT )
    {
        rightSpeed -= speed;
    }
    
    if ( mv->m_nButtons & IN_MOVERIGHT )
    {
        rightSpeed += speed;
    }

    if ( forwardSpeed != 0 || rightSpeed != 0 )
    {
        // See if the player is looking toward the top or the bottom
        Vector velocity;

        VectorScale( m_vecForward, forwardSpeed, velocity );
        VectorMA( velocity, rightSpeed, m_vecRight, velocity );

        velocity.NormalizeInPlace();

        Vector ladderUp;
        pLadder->ComputeLadderDir( ladderUp );
        ladderUp.NormalizeInPlace();


        bool ishorizontal = fabs( top.z - bottom.z ) < 64.0f ? true : false;

        float changeover = ishorizontal ? 0.0f : 0.3f;

        float factor = 1.0f;
        if ( velocity.z >= 0 )
        {
            float dotTop = ladderUp.Dot( velocity );
            if ( dotTop < -changeover )
            {
                // Aimed at bottom
                factor = -1.0f;
            }
        }
        else
        {
            float dotBottom = -ladderUp.Dot( velocity );
            if ( dotBottom > changeover )
            {
                factor = -1.0f;
            }
        }

        mv->m_vecVelocity = ClimbSpeed() * factor * ladderUp;
    }
    else
    {
        mv->m_vecVelocity.Init();
    }

    return LADDERMOVERET_ONLADDER;
}

void CZMGameMovement::StartForcedMove( bool mounting, float transit_speed, const Vector& goalpos, CFuncLadder* pLadder )
{
    LadderMove_t* lm = GetLadderMove();
    Assert( lm );

    // Already active, just ignore
    if ( lm->m_bForceLadderMove )
    {
        return;
    }

#if !defined( CLIENT_DLL )
    if ( pLadder )
    {
        pLadder->PlayerGotOn( GetZMPlayer() );

        // If the Ladder only wants to be there for automount checking, abort now
        if ( pLadder->DontGetOnLadder() )
            return;
    }
#endif

    // Use current player origin as start and new origin as dest
    lm->m_vecGoalPosition	= goalpos;
    lm->m_vecStartPosition	= mv->GetAbsOrigin();

    // Figure out how long it will take to make the gap based on transit_speed
    Vector delta = lm->m_vecGoalPosition - lm->m_vecStartPosition;

    float distance = delta.Length();
    
    Assert( transit_speed > 0.001f );

    // Compute time required to move that distance
    float transit_time = distance / transit_speed;
    if ( transit_time < 0.001f )
    {
        transit_time = 0.001f;
    }

    lm->m_bForceLadderMove	= true;
    lm->m_bForceMount		= mounting;

    lm->m_flStartTime		= gpGlobals->curtime;
    lm->m_flArrivalTime		= lm->m_flStartTime + transit_time;

    lm->m_hForceLadder.Set( pLadder );

    // Don't get stuck during this traversal since we'll just be slamming the player origin
    player->SetMoveType( MOVETYPE_NONE );
    player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
    player->SetSolid( SOLID_NONE );
    SetLadder( pLadder );
}

//-----------------------------------------------------------------------------
// Purpose: Returns false when finished
//-----------------------------------------------------------------------------
bool CZMGameMovement::ContinueForcedMove()
{
    LadderMove_t* lm = GetLadderMove();
    Assert( lm );
    Assert( lm->m_bForceLadderMove );

    // Suppress regular motion
    mv->m_flForwardMove = 0.0f;
    mv->m_flSideMove = 0.0f;
    mv->m_flUpMove = 0.0f;

    // How far along are we
    float frac = ( gpGlobals->curtime - lm->m_flStartTime ) / ( lm->m_flArrivalTime - lm->m_flStartTime );
    if ( frac > 1.0f )
    {
        lm->m_bForceLadderMove = false;
    }

    auto smoothstep = []( float x ) { return x * x * (3 - 2 * x); };

    frac = clamp( frac, 0.0f, 1.0f );

    frac = smoothstep( frac );

    // Move origin part of the way
    Vector delta = lm->m_vecGoalPosition - lm->m_vecStartPosition;

    // Compute interpolated position
    Vector org;
    VectorMA( lm->m_vecStartPosition, frac, delta, org );
    mv->SetAbsOrigin( org );

    // If finished moving, reset player to correct movetype (or put them on the ladder)
    if ( !lm->m_bForceLadderMove )
    {
        player->SetSolid( SOLID_BBOX );
        player->SetMoveType( MOVETYPE_WALK );

        if ( lm->m_bForceMount && lm->m_hForceLadder.Get() )
        {
            player->SetMoveType( MOVETYPE_LADDER );
            SetLadder( lm->m_hForceLadder );
        }

        // Zero out any velocity
        mv->m_vecVelocity.Init();
    }

    // Stil active
    return lm->m_bForceLadderMove;
}

bool CZMGameMovement::ExitLadderViaDismountNode( CFuncLadder* pLadder, bool strict )
{
    //
    // Find the best ladder exit node
    //

    // The player doesn't want to move anywhere, don't dismount.
    if ( !strict && mv->m_flForwardMove == 0.0f && mv->m_flSideMove == 0.0f )
    {
        return false;
    }

    float bestDot = -FLT_MAX;
    float bestDistance = FLT_MAX;
    Vector bestDest;
    bool found = false;


    bool bFoundBackup = false;
    Vector backupDest;
    float backupDist = FLT_MAX;

    Vector fwdXY = m_vecForward;
    fwdXY.z = 0.0f;
    fwdXY.NormalizeInPlace();


    Vector wishdir;
    for ( int i = 0; i < 2; i++ )
        wishdir[i] = m_vecForward[i] * mv->m_flForwardMove + m_vecRight[i] * mv->m_flSideMove;
    wishdir[2] = 0.0f;

    wishdir.NormalizeInPlace();



    int nDismounts = pLadder->GetDismountCount();

    for ( int i = 0; i < nDismounts; i++ )
    {
        auto* spot = pLadder->GetDismount( i );
        if ( !spot )
        {
            continue;
        }

        // See if it's valid to put the player there...
        Vector org = spot->GetAbsOrigin() + Vector( 0, 0, 1 );

        // Too far away.
        if ( org.DistToSqr( mv->GetAbsOrigin() ) > (LADDER_DISMOUNT_MAX_DIST*LADDER_DISMOUNT_MAX_DIST) )
        {
            continue;
        }


        trace_t tr;
        UTIL_TraceHull(
            org, 
            org, 
            GetPlayerMins( ( player->GetFlags() & FL_DUCKING ) ? true : false ),
            GetPlayerMaxs( ( player->GetFlags() & FL_DUCKING ) ? true : false ),
            MASK_PLAYERSOLID,
            player,
            COLLISION_GROUP_PLAYER_MOVEMENT,
            &tr );

        // Nope...
        if ( tr.startsolid )
        {
            continue;
        }

        //
        // Find the best dot product
        //
        Vector pos = mv->GetAbsOrigin();

        Vector dirToSpot = org - pos;
        dirToSpot.z = 0.0f;
        float dist = dirToSpot.NormalizeInPlace();

        float lookdot = dirToSpot.Dot( fwdXY );
        float movedot = dirToSpot.Dot( wishdir );

        //
        // We must be looking or moving that way to use that spot.
        //
        if ((lookdot > bestDot && lookdot > LADDER_DISMOUNT_DOT)
        ||	(movedot > bestDot && movedot > LADDER_DISMOUNT_DOT))
        {
            bestDest = org;
            bestDistance = dist;
            bestDot = MAX( movedot, lookdot );
            found = true;
        }

        if ( dist < backupDist )
        {
            backupDest = org;
            backupDist = dist;
            bFoundBackup = true;
        }
    }

    if ( found )
    {
        StartForcedMove( false, player->MaxSpeed(), bestDest, nullptr );
        return true;
    }


    if( bFoundBackup && strict )
    {
        StartForcedMove( false, player->MaxSpeed(), backupDest, nullptr );
        return true;
    }

    return false;
}

bool CZMGameMovement::IsForceMoveActive()
{
    LadderMove_t* lm = GetLadderMove();
    return lm->m_bForceLadderMove;
}

LadderMove_t* CZMGameMovement::GetLadderMove() const
{
    return &GetZMPlayer()->m_ZMLocal.m_LadderMove;
}

CFuncLadder* CZMGameMovement::GetLadder() const
{
    return GetZMPlayer()->m_ZMLocal.m_hLadder.Get();
}

void CZMGameMovement::SetLadder( CFuncLadder* pLadder )
{
    auto* pOldLadder = GetLadder();

    if ( pLadder != pOldLadder && pOldLadder )
    {
        pOldLadder->PlayerGotOff( player );
    }

    GetZMPlayer()->m_ZMLocal.m_hLadder.Set( pLadder );
}

void CZMGameMovement::ExitLadder()
{
    auto* pOldLadder = GetLadder();
    if ( pOldLadder )
    {
        pOldLadder->PlayerGotOff( player );
    }

    GetZMPlayer()->ExitLadder();
}

bool CZMGameMovement::LadderMove()
{
    LadderMoveRet_t ret = LADDERMOVERET_NO_LADDER;

    bool bInHL2LadderMove = IsForceMoveActive() || GetZMPlayer()->m_ZMLocal.m_hLadder.Get();


    bool valid = GetZMPlayer()->IsHuman() && GetZMPlayer()->IsAlive();

    if ( !valid )
    {
        ExitLadder();

        return LADDERMOVERET_NO_LADDER;
    }

    if ( !bInHL2LadderMove )
    {
        // The old fashioned brush ladder move.
        ret = LadderMove_Brush();
    }

    if ( ret == LADDERMOVERET_NO_LADDER )
    {
        // HL2 specific, func_ladder point entity one.
        ret = LadderMove_HL2();
    }


    if ( ret != LADDERMOVERET_ONLADDER )
    {
        ExitLadder();
    }

    return ret == LADDERMOVERET_ONLADDER;
}

/*
    NOTE: Remove this same shit from hl2/hl_gamemovement.cpp
*/

// Expose our interface.
static CZMGameMovement g_GameMovement;

IGameMovement* g_pGameMovement = static_cast<IGameMovement*>( &g_GameMovement );

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CZMGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );
