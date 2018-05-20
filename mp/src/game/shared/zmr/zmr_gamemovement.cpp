#include "cbase.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "convar.h"

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

#include "zmr_gamemovement.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern bool g_bMovementOptimizations;

static ConVar sv_autoladderdismount( "sv_autoladderdismount", "1", FCVAR_REPLICATED, "Automatically dismount from ladders when you reach the end (don't have to +USE)." );
static ConVar sv_ladderautomountdot( "sv_ladderautomountdot", "0.4", FCVAR_REPLICATED, "When auto-mounting a ladder by looking up its axis, this is the tolerance for looking now directly along the ladder axis." );

static ConVar sv_ladder_useonly( "sv_ladder_useonly", "0", FCVAR_REPLICATED, "If set, ladders can only be mounted by pressing +USE" );

#define USE_DISMOUNT_SPEED 100


#ifndef CLIENT_DLL
// This is a simple helper class to reserver a player sized hull at a spot, owned by the current player so that nothing
//  can move into this spot and cause us to get stuck when we get there
class CReservePlayerSpot : public CBaseEntity
{
	DECLARE_CLASS( CReservePlayerSpot, CBaseEntity )
public:
	static CReservePlayerSpot *ReserveSpot( CBasePlayer *owner, const Vector& org, const Vector& mins, const Vector& maxs, bool& validspot );

	virtual void Spawn();
};

CReservePlayerSpot *CReservePlayerSpot::ReserveSpot( 
	CBasePlayer *owner, const Vector& org, const Vector& mins, const Vector& maxs, bool& validspot )
{
	CReservePlayerSpot *spot = ( CReservePlayerSpot * )CreateEntityByName( "reserved_spot" );
	Assert( spot );

	spot->SetAbsOrigin( org );
	UTIL_SetSize( spot, mins, maxs );
	spot->SetOwnerEntity( owner );
	spot->Spawn();

	// See if spot is valid
	trace_t tr;
	UTIL_TraceHull(
		org, 
		org, 
		mins,
		maxs,
		MASK_PLAYERSOLID,
		owner,
		COLLISION_GROUP_PLAYER_MOVEMENT,
		&tr );

	validspot = !tr.startsolid;

	if ( !validspot )
	{
		Vector org2 = org + Vector( 0, 0, 1 );

		// See if spot is valid
		trace_t tr;
		UTIL_TraceHull(
			org2, 
			org2, 
			mins,
			maxs,
			MASK_PLAYERSOLID,
			owner,
			COLLISION_GROUP_PLAYER_MOVEMENT,
			&tr );
		validspot = !tr.startsolid;
	}

	return spot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReservePlayerSpot::Spawn()
{
	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );
	// Make entity invisible
	AddEffects( EF_NODRAW );
}

LINK_ENTITY_TO_CLASS( reserved_spot, CReservePlayerSpot );

#endif


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
                FullZMMove( sv_noclipspeed.GetFloat(), sv_noclipaccelerate.GetFloat() );
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

void CZMGameMovement::FullZMMove( float factor, float maxacceleration )
{
	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float maxspeed = sv_maxspeed.GetFloat() * factor;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
    
	/*if ( mv->m_nButtons & IN_SPEED )
	{
		factor /= 2.0f;
	}*/
	
	// Copy movement amounts
	float fmove = mv->m_flForwardMove * factor;
	float smove = mv->m_flSideMove * factor;
    
	if ( !(mv->m_nButtons & IN_ZM_OBSERVERMODE) )
	{
		forward[2] = 0;
		right[2] = 0;
	}

	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for ( int i = 0; i < 3; i++ )
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += mv->m_flUpMove * factor;


    // Let players go up/down with jump and duck.
    if ( mv->m_nButtons & IN_JUMP )
    {
        wishvel[2] += 300.0f * factor;
    }

    if ( mv->m_nButtons & IN_SPEED )
    {
        wishvel[2] -= 300.0f * factor;
    }


	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( maxacceleration > 0.0 )
	{
		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, maxacceleration );

		float spd = VectorLength( mv->m_vecVelocity );
		if (spd < 1.0f)
		{
			mv->m_vecVelocity.Init();
			return;
		}
		
		// Bleed off some speed, but if we have less than the bleed
		//  threshhold, bleed the theshold amount.
		float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;
		
		float friction = sv_friction.GetFloat();// * player->m_surfaceFriction;
				
		// Add the amount to the drop amount.
		float drop = control * friction * gpGlobals->frametime;

		// scale the velocity
		float newspeed = spd - drop;
		if (newspeed < 0)
			newspeed = 0;

		// Determine proportion of old speed we are using.
		newspeed /= spd;
		VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );
	}
	else
	{
		VectorCopy( wishvel, mv->m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	Vector out;
	VectorMA( mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, out );

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


	// Zero out velocity if in noaccel mode
	if ( maxacceleration < 0.0f )
	{
		mv->m_vecVelocity.Init();
	}
}

void CZMGameMovement::PlayerRoughLandingEffects( float fvol )
{
    if ( GetZMPlayer()->IsZM() )
        return;

    CGameMovement::PlayerRoughLandingEffects( fvol );
}

bool CZMGameMovement::LadderMove( void )
{
	if ( player->GetMoveType() == MOVETYPE_NOCLIP )
	{
		SetLadder( NULL );
		return false;
	}

	// If being forced to mount/dismount continue to act like we are on the ladder
	if ( IsForceMoveActive() && ContinueForcedMove() )
	{
		return true;
	}

	CFuncLadder *bestLadder = NULL;
	Vector bestOrigin( 0, 0, 0 );

	CFuncLadder *ladder = GetLadder();

	// Something 1) deactivated the ladder...  or 2) something external applied
	//  a force to us.  In either case  make the player fall, etc.
	if ( ladder && 
		 ( !ladder->IsEnabled() ||
		 ( player->GetBaseVelocity().LengthSqr() > 1.0f ) ) )
	{
		GetHL2Player()->ExitLadder();
		ladder = NULL;
	}

	if ( !ladder )
	{
		Findladder( 64.0f, &bestLadder, bestOrigin, NULL );
	}

/*#if !defined (CLIENT_DLL)
	if( !ladder && bestLadder && sv_ladder_useonly.GetBool() )
	{
		GetHL2Player()->DisplayLadderHudHint();
	}
#endif*/

	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed = buttonsChanged & mv->m_nButtons;
	bool pressed_use = ( buttonsPressed & IN_USE ) ? true : false;

	// If I'm already moving on a ladder, use the previous ladder direction
	if ( !ladder && !pressed_use )
	{
		// If flying through air, allow mounting ladders if we are facing < 15 degress from the ladder and we are close
		if ( !ladder && !sv_ladder_useonly.GetBool() )
		{
			// Tracker 6625:  Don't need to be leaping to auto mount using this method...
			// But if we are on the ground, then we must not be backing into the ladder (Tracker 12961)
			bool onground = player->GetGroundEntity() ? true : false;
			if ( !onground || ( mv->m_flForwardMove > 0.0f ) )
			{
				if ( CheckLadderAutoMountCone( bestLadder, bestOrigin, 15.0f, 32.0f ) )
				{
					return true;
				}
			}
			
			// Pressing forward while looking at ladder and standing (or floating) near a mounting point
			if ( mv->m_flForwardMove > 0.0f )
			{
				if ( CheckLadderAutoMountEndPoint( bestLadder, bestOrigin ) )
				{
					return true;
				}
			}
		}

		return false;
	}

	if ( !ladder && 
		LookingAtLadder( bestLadder ) &&
		CheckLadderAutoMount( bestLadder, bestOrigin ) )
	{
		return true;
	}

	// Reassign the ladder
	ladder = GetLadder();
	if ( !ladder )
	{
		return false;
	}

	// Don't play the deny sound
	if ( pressed_use )
	{
		//GetHL2Player()->m_bPlayUseDenySound = false;
	}

	// Make sure we are on the ladder
	player->SetMoveType( MOVETYPE_LADDER );
	player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

	player->SetGravity( 0.0f );
	
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
	
	if ( mv->m_nButtons & IN_JUMP )
	{
		player->SetMoveType( MOVETYPE_WALK );
		// Remove from ladder
		SetLadder( NULL );

		// Jump in view direction
		Vector jumpDir = m_vecForward;

		// unless pressing backward or something like that
		if ( mv->m_flForwardMove < 0.0f )
		{
			jumpDir = -jumpDir;
		}

		VectorNormalize( jumpDir );

		VectorScale( jumpDir, MAX_CLIMB_SPEED, mv->m_vecVelocity );
		// Tracker 13558:  Don't add any extra z velocity if facing downward at all
		if ( m_vecForward.z >= 0.0f )
		{
			mv->m_vecVelocity.z = mv->m_vecVelocity.z + 50;
		}
		return false;
	}

	if ( forwardSpeed != 0 || rightSpeed != 0 )
	{
		// See if the player is looking toward the top or the bottom
		Vector velocity;

		VectorScale( m_vecForward, forwardSpeed, velocity );
		VectorMA( velocity, rightSpeed, m_vecRight, velocity );

		VectorNormalize( velocity );

		Vector ladderUp;
		ladder->ComputeLadderDir( ladderUp );
		VectorNormalize( ladderUp );

		Vector topPosition;
		Vector bottomPosition;

		ladder->GetTopPosition( topPosition );
		ladder->GetBottomPosition( bottomPosition );

		// Check to see if we've mounted the ladder in a bogus spot and, if so, just fall off the ladder...
		float dummyt = 0.0f;
		float distFromLadderSqr = CalcDistanceSqrToLine( mv->GetAbsOrigin(), topPosition, bottomPosition, &dummyt );
		if ( distFromLadderSqr > 36.0f )
		{
			// Uh oh, we fell off zee ladder...
			player->SetMoveType( MOVETYPE_WALK );
			// Remove from ladder
			SetLadder( NULL );
			return false;
		}

		bool ishorizontal = fabs( topPosition.z - bottomPosition.z ) < 64.0f ? true : false;

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

#ifdef _XBOX
		if( sv_ladders_useonly.GetBool() )
		{
			// Stick up climbs up, stick down climbs down. No matter which way you're looking.
			if ( mv->m_nButtons & IN_FORWARD )
			{
				factor = 1.0f;
			}
			else if( mv->m_nButtons & IN_BACK )
			{
				factor = -1.0f;
			}
		}
#endif//_XBOX

		mv->m_vecVelocity = MAX_CLIMB_SPEED * factor * ladderUp;
	}
	else
	{
		mv->m_vecVelocity.Init();
	}

	return true;
}

bool CZMGameMovement::LookingAtLadder( CFuncLadder *ladder )
{
	if ( !ladder )
	{
		return false;
	}

	// Get ladder end points
	Vector top, bottom;
	ladder->GetTopPosition( top );
	ladder->GetBottomPosition( bottom );

	// Find closest point on ladder to player (could be an endpoint)
	Vector closest;
	CalcClosestPointOnLineSegment( mv->GetAbsOrigin(), bottom, top, closest, NULL );

	// Flatten our view direction to 2D
	Vector flatForward = m_vecForward;
	flatForward.z = 0.0f;

	// Because the ladder itself is not a solid, the player's origin may actually be 
	// permitted to pass it, and that will screw up our dot product.
	// So back up the player's origin a bit to do the facing calculation.
	Vector vecAdjustedOrigin = mv->GetAbsOrigin() - 8.0f * flatForward;

	// Figure out vector from player to closest point on ladder
	Vector vecToLadder = closest - vecAdjustedOrigin;

	// Flatten it to 2D
	Vector flatLadder = vecToLadder;
	flatLadder.z = 0.0f;

	// Normalize the vectors (unnecessary)
	VectorNormalize( flatLadder );
	VectorNormalize( flatForward );

	// Compute dot product to see if forward is in same direction as vec to ladder
	float facingDot = flatForward.Dot( flatLadder );

	float requiredDot = ( sv_ladder_useonly.GetBool() ) ? -0.99 : 0.0;

	// Facing same direction if dot > = requiredDot...
	bool facingladder = ( facingDot >= requiredDot );

	return facingladder;
}

void CZMGameMovement::Findladder( float maxdist, CFuncLadder **ppLadder, Vector& ladderOrigin, const CFuncLadder *skipLadder )
{
	CFuncLadder *bestLadder = NULL;
	float bestDist = MAX_COORD_INTEGER;
	Vector bestOrigin;

	bestOrigin.Init();

	float maxdistSqr = maxdist * maxdist;


	int c = CFuncLadder::GetLadderCount();
	for ( int i = 0 ; i < c; i++ )
	{
		CFuncLadder *ladder = CFuncLadder::GetLadder( i );

		if ( !ladder->IsEnabled() )
			continue;

		if ( skipLadder && ladder == skipLadder )
			continue;

		Vector topPosition;
		Vector bottomPosition;

		ladder->GetTopPosition( topPosition );
		ladder->GetBottomPosition( bottomPosition );

		Vector closest;
		CalcClosestPointOnLineSegment( mv->GetAbsOrigin(), bottomPosition, topPosition, closest, NULL );

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
			 tr.m_pEnt != ladder )
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
				 tr.m_pEnt != ladder &&
				 !tr.m_pEnt->IsSolidFlagSet( FSOLID_TRIGGER ) )
			{
				continue;
			}
		}

		// See if this is the best one so far
		if ( distSqr < bestDist )
		{
			bestDist = distSqr;
			bestLadder = ladder;
			bestOrigin = closest;
		}
	}

	// Return best ladder spot
	*ppLadder = bestLadder;
	ladderOrigin = bestOrigin;

}

static bool NearbyDismountLessFunc( const NearbyDismount_t& lhs, const NearbyDismount_t& rhs )
{
	return lhs.distSqr < rhs.distSqr;
}

void CZMGameMovement::GetSortedDismountNodeList( const Vector &org, float radius, CFuncLadder *ladder, CUtlRBTree< NearbyDismount_t, int >& list )
{
	float radiusSqr = radius * radius;

	int i;
	int c = ladder->GetDismountCount();
	for ( i = 0; i < c; i++ )
	{
		CInfoLadderDismount *spot = ladder->GetDismount( i );
		if ( !spot )
			continue;

		float distSqr = ( spot->GetAbsOrigin() - org ).LengthSqr();
		if ( distSqr > radiusSqr )
			continue;

		NearbyDismount_t nd;
		nd.dismount = spot;
		nd.distSqr = distSqr;

		list.Insert( nd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//			*ladder - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMGameMovement::ExitLadderViaDismountNode( CFuncLadder *ladder, bool strict, bool useAlternate )
{
	// Find the best ladder exit node
	float bestDot = -99999.0f;
	float bestDistance = 99999.0f;
	Vector bestDest;
	bool found = false;

	// For 'alternate' dismount
	bool foundAlternate = false;
	Vector alternateDest;
	float alternateDist = 99999.0f;

	CUtlRBTree< NearbyDismount_t, int >	nearbyDismounts( 0, 0, NearbyDismountLessFunc );

	GetSortedDismountNodeList( mv->GetAbsOrigin(), 100.0f, ladder, nearbyDismounts );

	int i;

	for ( i = nearbyDismounts.FirstInorder(); i != nearbyDismounts.InvalidIndex() ; i = nearbyDismounts.NextInorder( i ) )
	{
		CInfoLadderDismount *spot = nearbyDismounts[ i ].dismount;
		if ( !spot )
		{
			Assert( !"What happened to the spot!!!" );
			continue;
		}

		// See if it's valid to put the player there...
		Vector org = spot->GetAbsOrigin() + Vector( 0, 0, 1 );

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

		// Find the best dot product
		Vector vecToSpot = org - ( mv->GetAbsOrigin() + player->GetViewOffset() );
		vecToSpot.z = 0.0f;
		float d = VectorNormalize( vecToSpot );

		float dot = vecToSpot.Dot( m_vecForward );

		// We're not facing at it...ignore
		if ( dot < 0.5f )
		{
			if( useAlternate && d < alternateDist )
			{
				alternateDest = org;
				alternateDist = d;
				foundAlternate = true;
			}

			continue;
		}

		if ( dot > bestDot )
		{
			bestDest = org;
			bestDistance = d;
			bestDot = dot;
			found = true;
		}
	}

	if ( found )
	{
		// Require a more specific 
		if ( strict && 
			( ( bestDot < 0.7f ) || ( bestDistance > 40.0f ) ) )
		{
			return false;
		}

		StartForcedMove( false, player->MaxSpeed(), bestDest, NULL );
		return true;
	}

	if( useAlternate )
	{
		// Desperate. Don't refuse to let a person off of a ladder if it can be helped. Use the
		// alternate dismount if there is one.
		if( foundAlternate && alternateDist <= 60.0f )
		{
			StartForcedMove( false, player->MaxSpeed(), alternateDest, NULL );
			return true;
		}
	}

	return false;
}

bool CZMGameMovement::CheckLadderAutoMountEndPoint( CFuncLadder *ladder, const Vector& bestOrigin )
{
	// See if we're really near an endpoint
	if ( !ladder )
		return false;

	Vector top, bottom;
	ladder->GetTopPosition( top );
	ladder->GetBottomPosition( bottom );

	float d1, d2;

	d1 = ( top - mv->GetAbsOrigin() ).LengthSqr();
	d2 = ( bottom - mv->GetAbsOrigin() ).LengthSqr();

	if ( d1 > 16 * 16 && d2 > 16 * 16 )
		return false;

	Vector ladderAxis;

	if ( d1 < 16 * 16 )
	{
		// Close to top
		ladderAxis = bottom - top;
	}
	else
	{
		ladderAxis = top - bottom;
	}

	VectorNormalize( ladderAxis );

	if ( ladderAxis.Dot( m_vecForward ) > sv_ladderautomountdot.GetFloat() )
	{
		StartForcedMove( true, player->MaxSpeed(), bestOrigin, ladder );
		return true;
	}

	return false;
}

bool CZMGameMovement::CheckLadderAutoMountCone( CFuncLadder *ladder, const Vector& bestOrigin, float maxAngleDelta, float maxDistToLadder )
{
	// Never 'back' onto ladders or stafe onto ladders
	if ( ladder != NULL && 
		( mv->m_flForwardMove > 0.0f ) )
	{
		Vector top, bottom;
		ladder->GetTopPosition( top );
		ladder->GetBottomPosition( bottom );

		Vector ladderAxis = top - bottom;
		VectorNormalize( ladderAxis );

		Vector probe = mv->GetAbsOrigin();

		Vector closest;
		CalcClosestPointOnLineSegment( probe, bottom, top, closest, NULL );

		Vector vecToLadder = closest - probe;

		float dist = VectorNormalize( vecToLadder );

		Vector flatLadder = vecToLadder;
		flatLadder.z = 0.0f;
		Vector flatForward = m_vecForward;
		flatForward.z = 0.0f;

		VectorNormalize( flatLadder );
		VectorNormalize( flatForward );

		float facingDot = flatForward.Dot( flatLadder );
		float angle = acos( facingDot ) * 180 / M_PI;

		bool closetoladder = ( dist != 0.0f && dist < maxDistToLadder ) ? true : false;
		bool reallyclosetoladder = ( dist != 0.0f && dist < 4.0f ) ? true : false;

		bool facingladderaxis = ( angle < maxAngleDelta ) ? true : false;
		bool facingalongaxis = ( (float)fabs( ladderAxis.Dot( m_vecForward ) ) > sv_ladderautomountdot.GetFloat() ) ? true : false;
#if 0
		Msg( "close %i length %.3f maxdist %.3f facing %.3f dot %.3f ang %.3f\n",
			closetoladder ? 1 : 0,
			dist,
			maxDistToLadder,
			(float)fabs( ladderAxis.Dot( m_vecForward ) ),
			facingDot, 
			angle);
#endif

		// Tracker 21776:  Don't mount ladders this way if strafing
		bool strafing = ( fabs( mv->m_flSideMove ) < 1.0f ) ? false : true;

		if ( ( ( facingDot > 0.0f && !strafing ) || facingalongaxis  ) && 
			( facingladderaxis || reallyclosetoladder ) && 
			closetoladder )
		{
			StartForcedMove( true, player->MaxSpeed(), bestOrigin, ladder );
			return true;
		}
	}

	return false;
}

bool CZMGameMovement::CheckLadderAutoMount( CFuncLadder *ladder, const Vector& bestOrigin )
{
#if !defined( CLIENT_DLL )

	if ( ladder != NULL )
	{
		StartForcedMove( true, player->MaxSpeed(), bestOrigin, ladder );
		return true;
	}

#endif
	return false;
}

void CZMGameMovement::StartForcedMove( bool mounting, float transit_speed, const Vector& goalpos, CFuncLadder *ladder )
{
	LadderMove_t* lm = GetLadderMove();
	Assert( lm );
	// Already active, just ignore
	if ( lm->m_bForceLadderMove )
	{
		return;
	}

#ifndef CLIENT_DLL
	if ( ladder )
	{
		ladder->PlayerGotOn( GetHL2Player() );

		// If the Ladder only wants to be there for automount checking, abort now
		if ( ladder->DontGetOnLadder() )
			return;
	}
		
	// Reserve goal slot here
	bool valid = false;
	lm->m_hReservedSpot = CReservePlayerSpot::ReserveSpot( 
		player, 
		goalpos, 
		GetPlayerMins( ( player->GetFlags() & FL_DUCKING ) ? true : false ), 
		GetPlayerMaxs( ( player->GetFlags() & FL_DUCKING ) ? true : false ), 
		valid );
	if ( !valid )
	{
		// FIXME:  Play a deny sound?
		if ( lm->m_hReservedSpot )
		{
			UTIL_Remove( lm->m_hReservedSpot );
			lm->m_hReservedSpot = NULL;
		}
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

#ifndef CLIENT_DLL
	lm->m_hForceLadder.Set( ladder );
#endif

	// Don't get stuck during this traversal since we'll just be slamming the player origin
	player->SetMoveType( MOVETYPE_NONE );
	player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
	player->SetSolid( SOLID_NONE );
	SetLadder( ladder );

	// Debounce the use key
	SwallowUseKey();
}

void CZMGameMovement::SwallowUseKey()
{
	mv->m_nOldButtons |= IN_USE;
	player->m_afButtonPressed &= ~IN_USE;

	//GetHL2Player()->m_bPlayUseDenySound = false;
}

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
#if !defined( CLIENT_DLL )
		// Remove "reservation entity"
		if ( lm->m_hReservedSpot )
		{
			UTIL_Remove( lm->m_hReservedSpot );
			lm->m_hReservedSpot = NULL;
		}
#endif
	}

	frac = clamp( frac, 0.0f, 1.0f );

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

		if ( lm->m_bForceMount && lm->m_hForceLadder != NULL )
		{
			player->SetMoveType( MOVETYPE_LADDER );

#ifdef CLIENT_DLL
            CFuncLadder* ladder = (C_FuncLadder*)lm->m_hForceLadder.Get();
#else
            CFuncLadder* ladder = lm->m_hForceLadder.Get();
#endif
			SetLadder( ladder );
		}

		// Zero out any velocity
		mv->m_vecVelocity.Init();
	}

	// Stil active
	return lm->m_bForceLadderMove;
}

void CZMGameMovement::FullLadderMove()
{
#if !defined( CLIENT_DLL )
    CFuncLadder *ladder = GetLadder();
    Assert( ladder );
    if ( !ladder )
    {
        return;
    }

    CheckWater();

    // Was jump button pressed?  If so, don't do anything here
    if ( mv->m_nButtons & IN_JUMP )
    {
        CheckJumpButton();
        return;
    }
    else
    {
        mv->m_nOldButtons &= ~IN_JUMP;
    }

    player->SetGroundEntity( NULL );

    // Remember old positions in case we cancel this movement
    Vector oldVelocity	= mv->m_vecVelocity;
    Vector oldOrigin	= mv->GetAbsOrigin();

    Vector topPosition;
    Vector bottomPosition;

    ladder->GetTopPosition( topPosition );
    ladder->GetBottomPosition( bottomPosition );

    // Compute parametric distance along ladder vector...
    float oldt;
    CalcDistanceSqrToLine( mv->GetAbsOrigin(), topPosition, bottomPosition, &oldt );
    
    // Perform the move accounting for any base velocity.
    VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
    TryPlayerMove();
    VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

    // Pressed buttons are "changed(xor)" and'ed with the mask of currently held buttons
    int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
    int buttonsPressed = buttonsChanged & mv->m_nButtons;
    bool pressed_use = ( buttonsPressed & IN_USE ) ? true : false;
    bool pressing_forward_or_side = mv->m_flForwardMove != 0.0f || mv->m_flSideMove != 0.0f;

    Vector ladderVec = topPosition - bottomPosition;
    float LadderLength = VectorNormalize( ladderVec );
    // This test is not perfect by any means, but should help a bit
    bool moving_along_ladder = false;
    if ( pressing_forward_or_side )
    {
        float fwdDot = m_vecForward.Dot( ladderVec );
        if ( fabs( fwdDot ) > 0.9f )
        {
            moving_along_ladder = true;
        }
    }

    // Compute parametric distance along ladder vector...
    float newt;
    CalcDistanceSqrToLine( mv->GetAbsOrigin(), topPosition, bottomPosition, &newt );

    // Fudge of 2 units
    float tolerance = 1.0f / LadderLength;

    bool wouldleaveladder = false;
    // Moving pPast top or bottom?
    if ( newt < -tolerance )
    {
        wouldleaveladder = newt < oldt;
    }
    else if ( newt > ( 1.0f + tolerance ) )
    {
        wouldleaveladder = newt > oldt;
    }

    // See if we are near the top or bottom but not moving
    float dist1sqr, dist2sqr;

    dist1sqr = ( topPosition - mv->GetAbsOrigin() ).LengthSqr();
    dist2sqr = ( bottomPosition - mv->GetAbsOrigin() ).LengthSqr();

    float dist = min( dist1sqr, dist2sqr );
    bool neardismountnode = ( dist < 16.0f * 16.0f ) ? true : false;
    float ladderUnitsPerTick = ( MAX_CLIMB_SPEED * gpGlobals->interval_per_tick );
    bool neardismountnode2 = ( dist < ladderUnitsPerTick * ladderUnitsPerTick ) ? true : false;

    // Really close to node, cvar is set, and pressing a key, then simulate a +USE
    bool auto_dismount_use = ( neardismountnode2 && 
                                sv_autoladderdismount.GetBool() && 
                                pressing_forward_or_side && 
                                !moving_along_ladder );

    bool fully_underwater = ( player->GetWaterLevel() == WL_Eyes ) ? true : false;

    // If the user manually pressed use or we're simulating it, then use_dismount will occur
    bool use_dismount = pressed_use || auto_dismount_use;

    if ( fully_underwater && !use_dismount )
    {
        // If fully underwater, we require looking directly at a dismount node 
        ///  to "float off" a ladder mid way...
        if ( ExitLadderViaDismountNode( ladder, true ) )
        {
            // See if they +used a dismount point mid-span..
            return;
        }
    }

    // If the movement would leave the ladder and they're not automated or pressing use, disallow the movement
    if ( !use_dismount )
    {
        if ( wouldleaveladder )
        {
            // Don't let them leave the ladder if they were on it
            mv->m_vecVelocity = oldVelocity;
            mv->SetAbsOrigin( oldOrigin );
        }
        return;
    }

    // If the move would not leave the ladder and we're near close to the end, then just accept the move
    if ( !wouldleaveladder && !neardismountnode )
    {
        // Otherwise, if the move would leave the ladder, disallow it.
        if ( pressed_use )
        {
            if ( ExitLadderViaDismountNode( ladder, false, IsXbox() ) )
            {
                // See if they +used a dismount point mid-span..
                return;
            }

            player->SetMoveType( MOVETYPE_WALK );
            player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
            SetLadder( NULL );
            //GetHL2Player()->m_bPlayUseDenySound = false;

            // Dismount with a bit of velocity in facing direction
            VectorScale( m_vecForward, USE_DISMOUNT_SPEED, mv->m_vecVelocity );
            mv->m_vecVelocity.z = 50;
        }
        return;
    }

    // Debounce the use key
    if ( pressed_use )
    {
        SwallowUseKey();
    }

    // Try auto exit, if possible
    if ( ExitLadderViaDismountNode( ladder, false, pressed_use ) )
    {
        return;
    }

    if ( wouldleaveladder )
    {
        // Otherwise, if the move would leave the ladder, disallow it.
        if ( pressed_use )
        {
            player->SetMoveType( MOVETYPE_WALK );
            player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
            SetLadder( NULL );

            // Dismount with a bit of velocity in facing direction
            VectorScale( m_vecForward, USE_DISMOUNT_SPEED, mv->m_vecVelocity );
            mv->m_vecVelocity.z = 50;
        }
        else
        {
            mv->m_vecVelocity = oldVelocity;
            mv->SetAbsOrigin( oldOrigin );
        }
    }
#endif
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
