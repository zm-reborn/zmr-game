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
#include "zmr/zmr_player.h"
#else
#include "zmr/c_zmr_player.h"
#endif

#include "zmr/zmr_softcollisions.h"
#include "zmr_gamemovement.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_maxbunnyhopspeed( "zm_sv_maxbunnyhopspeed", "300", FCVAR_REPLICATED | FCVAR_NOTIFY );


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
                // Pass our hit player to soft collisions.
                GetZMSoftCollisions()->OnPlayerCollide( m_pMe, pEnt );
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
			SetGroundEntity( NULL );
		}
	}

	// Store off the starting water level
	m_nOldWaterLevel = player->GetWaterLevel();

	// If we are not on ground, store off how fast we are moving down
	if ( player->GetGroundEntity() == NULL )
	{
		player->m_Local.m_flFallVelocity = -mv->m_vecVelocity[ 2 ];
	}

	m_nOnLadder = 0;

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
		SetGroundEntity( NULL );
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
				SetGroundEntity( NULL );
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
        SetGroundEntity( NULL );

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
    if (player->GetGroundEntity() == NULL)
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
    SetGroundEntity( NULL );
    
    player->PlayStepSound( (Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true );
    
    //MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );
    // ZMRCHANGE
    GetZMPlayer()->DoAnimationEvent( PLAYERANIMEVENT_JUMP );

    float flGroundFactor = 1.0f;
    if (player->m_pSurfaceData)
    {
        flGroundFactor = player->m_pSurfaceData->game.jumpFactor; 
    }

    float flMul;
    if ( g_bMovementOptimizations )
    {
#if defined(HL2_DLL) || defined(HL2_CLIENT_DLL)
        Assert( GetCurrentGravity() == 600.0f );
        flMul = 160.0f;	// approx. 21 units.
#else
        Assert( GetCurrentGravity() == 800.0f );
        flMul = 268.3281572999747f;
#endif

    }
    else
    {
        flMul = sqrt(2 * GetCurrentGravity() * GAMEMOVEMENT_JUMP_HEIGHT);
    }

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

/*
    NOTE: Remove this same shit from hl2/hl_gamemovement.cpp
*/

// Expose our interface.
static CZMGameMovement g_GameMovement;

IGameMovement* g_pGameMovement = static_cast<IGameMovement*>( &g_GameMovement );

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CZMGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );
