#include "cbase.h"
#include "team.h"
//#include "baseplayer.h"

#include "player_pickup.h"

#include "predicted_viewmodel.h"
#include "filesystem.h"
#include "EntityFlame.h"
#include "in_buttons.h"
#include "trains.h"
#include "info_camera_link.h"
#include <vphysics/player_controller.h>
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "soundenvelope.h"
#include "eventqueue.h"

#include "zmr_rejoindata.h"
#include "npcs/zmr_zombiebase.h"
#include "zmr_player_ragdoll.h"
#include "zmr_viewmodel.h"
#include "zmr_entities.h"
#include "zmr_gamerules.h"
#include "zmr_playermodels.h"
#include "weapons/zmr_fistscarry.h"
#include "zmr_ammodef.h"
#include "zmr_resource_system.h"
#include "zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define VMHANDS_FALLBACKMODEL   "models/weapons/c_arms_citizen.mdl"



extern int TrainSpeed( int iSpeed, int iMax );


static ConVar zm_sv_modelchangedelay( "zm_sv_modelchangedelay", "6", FCVAR_NOTIFY | FCVAR_ARCHIVE, "", true, 0.0f, false, 0.0f );

ConVar zm_sv_antiafk( "zm_sv_antiafk", "90", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If the player is AFK for this many seconds, put them into spectator mode. 0 = disable" );


static ConVar zm_sv_npcheadpushoff( "zm_sv_npcheadpushoff", "200", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How much force is applied to the player when standing on an NPC." );

// This is for maps that use the OnBreak trick to name the ZM. This does not work in ZMR anymore.
static ConVar zm_sv_comp_zmtargetname( "zm_sv_comp_zmtargetname", "", 0, "Gives the ZM a targetname on spawn. This is for compatibility with old maps!!!" );


extern ConVar zm_sv_antiafk_punish;

ConVar zm_sv_flashlightdrainrate( "zm_sv_flashlightdrainrate", "0.6", FCVAR_NOTIFY, "How fast the flashlight battery drains per second. (out of 100)" ); // Originally 0.4
ConVar zm_sv_flashlightrechargerate( "zm_sv_flashlightrechargerate", "2.5", FCVAR_NOTIFY, "How fast the flashlight battery recharges per second. (out of 100)" ); // Originally 0.1



void* SendProxy_SendNonLocalDataTable( const SendProp* pProp, const void* pStruct, const void* pVarData, CSendProxyRecipients* pRecipients, int objectID )
{
    pRecipients->SetAllRecipients();
    pRecipients->ClearRecipient( objectID - 1 );
    return (void*)pVarData;
}

BEGIN_SEND_TABLE_NOBASE( CZMPlayer, DT_ZMLocalPlayerExclusive )
    // Send high-resolution for local
    SendPropVector( SENDINFO( m_vecOrigin ), -1,  SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

    // Is there any point to sending pitch to local and not yaw?
    //SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CZMPlayer, DT_ZMNonLocalPlayerExclusive )
    // Send low-resolution for non-local
    SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

    SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
    SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 1 ), 10, SPROP_CHANGES_OFTEN ),

    SendPropInt( SENDINFO( m_nMuzzleFlashParity ), EF_MUZZLEFLASH_BITS, SPROP_UNSIGNED ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CZMPlayer, DT_ZM_Player )
    SendPropDataTable( SENDINFO_DT( m_ZMLocal ), &REFERENCE_SEND_TABLE( DT_ZM_PlyLocal ), SendProxy_SendLocalDataTable ),

    SendPropInt( SENDINFO( m_iSpawnInterpCounter ), 4, SPROP_UNSIGNED ),
    SendPropEHandle( SENDINFO( m_hRagdoll ) ),

    // Data that only gets sent to the local player.
    SendPropDataTable( "zmlocaldata", 0, &REFERENCE_SEND_TABLE(DT_ZMLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
    // Data that gets sent to all other players
    SendPropDataTable( "zmnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_ZMNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),


    // Other players' water level is networked for animations.
    SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),
    SendPropExclude( "DT_LocalPlayerExclusive", "m_nWaterLevel" ),
    
    // We only want to send it to other players.
    SendPropExclude( "DT_BaseAnimating", "m_nMuzzleFlashParity" ),

    
    SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
    SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
    SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

    // playeranimstate and clientside animation takes care of these on the client
    SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

    SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
    SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
    SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),
END_SEND_TABLE()


BEGIN_DATADESC( CZMPlayer )
    DEFINE_INPUTFUNC( FIELD_VOID, "ForceDropPhysObjects", InputForceDropPhysObjects ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( player, CZMPlayer );
PRECACHE_REGISTER( player );


CZMPlayerAnimState* CreateZMPlayerAnimState( CZMPlayer* pPlayer );

CZMPlayer::CZMPlayer()
{
    m_angEyeAngles.Init();
    m_iSpawnInterpCounter = 0;
    m_bIsFireBulletsRecursive = false;
    m_iLastWeaponFireUsercmd = 0;
    m_bEnterObserver = false;
    m_flNextModelChangeTime = 0.0f;
    m_flNextVoiceLineTime = 0.0f;
    m_flInterpNPCTime = 0.0f;
    m_ServerWepData.Reset();


    BaseClass::ChangeTeam( 0 );


    m_pPlayerAnimState = CreateZMPlayerAnimState( this );
    UseClientSideAnimation();

    SetResources( 0 );

    m_flLastActivity = gpGlobals->curtime;
    m_flLastActivityWarning = 0.0f;

    
    SetWeaponSlotFlags( 0 );


    m_flZMMoveSpeed = 1600.0f;
    m_flZMMoveAccel = 5.0f;
    m_flZMMoveDecel = 4.0f;
}

CZMPlayer::~CZMPlayer( void )
{
    m_pPlayerAnimState->Release();
}

void CZMPlayer::Precache()
{
    // Precache register makes sure we are already precached.
    if ( !IsPrecacheAllowed() ) return;


    BaseClass::Precache();

    // Needs to be precached for thirdperson flashlight beam.
    PrecacheModel ( "sprites/glow01.vmt" );

    PrecacheModel( VMHANDS_FALLBACKMODEL );

    PrecacheScriptSound( "ZMPlayer.PickupWeapon" );
    PrecacheScriptSound( "ZMPlayer.PickupAmmo" );

	PrecacheScriptSound( "ZMPlayer.FlashLightOn" );
	PrecacheScriptSound( "ZMPlayer.FlashLightOff" );

	PrecacheScriptSound( "HL2Player.SprintNoPower" );
	PrecacheScriptSound( "HL2Player.SprintStart" );
	PrecacheScriptSound( "HL2Player.UseDeny" );
	PrecacheScriptSound( "HL2Player.PickupWeapon" );
	PrecacheScriptSound( "HL2Player.TrainUse" );
	PrecacheScriptSound( "HL2Player.Use" );
	PrecacheScriptSound( "HL2Player.BurnPain" );


    ZMGetPlayerModels()->LoadModelsFromFile();

    if ( !ZMGetPlayerModels()->PrecachePlayerModels() )
    {
        Warning( "WARNING: No player models precached! Crash inbound!\n" );
    }
}

void CZMPlayer::UpdateOnRemove()
{
    if ( m_hRagdoll.Get() )
    {
        UTIL_RemoveImmediate( m_hRagdoll.Get() );
        m_hRagdoll.Set( nullptr );
    }

    BaseClass::UpdateOnRemove();
}

void CZMPlayer::NoteWeaponFired()
{
    Assert( m_pCurrentCommand );
    if ( m_pCurrentCommand )
    {
        m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
    }
}

void CZMPlayer::PreThink()
{
    // Erase our denied ammo.
    m_vAmmoDenied.Purge();


    if ( m_afButtonLast != m_nButtons )
    {
        m_flLastActivity = gpGlobals->curtime;
    }


    if ( IsCloseToAFK() && ZMRules()->CanInactivityPunish( this ) )
    {
        if ( IsAFK() )
        {
            ZMRules()->PunishInactivity( this );
        }
        else if ( (m_flLastActivityWarning + 1.0f) < gpGlobals->curtime )
        {
            ClientPrint( this, HUD_PRINTCENTER, "You are about to get punished for being AFK!" );

            m_flLastActivityWarning = gpGlobals->curtime;
        }
    }


    if ( IsZM() )
    {
        g_ZMResourceSystem.GainResources( this );
    }

    if ( IsAlive() )
    {
        UpdateAccuracyRatio();

        UpdateFlashlight();

        // Force player off the NPCs head!
        if ( zm_sv_npcheadpushoff.GetFloat() != 0.0f && GetGroundEntity() && !GetGroundEntity()->IsStandable() && GetGroundEntity()->IsBaseZombie() )
        {
            // A hack to keep pushing the player in the same direction they're looking at.
            // People wanting to crowd surf will get angry otherwise.
            Vector vec;
            AngleVectors( QAngle( 0.0f, GetAbsAngles().y, 0.0f ), &vec );

            PushAway( GetAbsOrigin() + vec * -10.0f, zm_sv_npcheadpushoff.GetFloat() );
        }
    }


    // Fixes player not changing target after the first target has died.
    if ( GetObserverMode() > OBS_MODE_FREEZECAM )
        CheckObserverSettings();


    // Baseclass think
    PreThink_HL2();



    SetMaxSpeed( ZM_WALK_SPEED );
    State_PreThink();

    // Reset bullet force accumulator, only lasts one frame
    m_vecTotalBulletForce = vec3_origin;
}

//
// This is the base prethink
//
void CZMPlayer::PreThink_HL2()
{
	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		VPROF( "CZMPlayer::PreThink-Vehicle" );
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();

		WaterMove();	
		return;
	}

	ItemPreFrame();

	WaterMove();

	// checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	if ( m_lifeState >= LIFE_DYING )
	{
		PlayerDeathThink();
		return;
	}


    UpdateControllableTrain();

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}
}

//
// Certain func_tracktrains can be controlled
// Literally the only map to use this is zm_ballworks
//
void CZMPlayer::UpdateControllableTrain()
{
    bool bWasOnTrain = ( m_afPhysicsFlags & PFLAG_DIROVERRIDE ) != 0;
    bool bOnTrain = bWasOnTrain && IsAlive() && IsHuman();

    int trainFlags = 0;


	// Train speed control
	if ( bOnTrain )
	{
		auto* pTrain = dynamic_cast<CFuncTrackTrain*>( GetGroundEntity() );
		float vel = 0.0f;


		if ( !pTrain )
		{
            // Trace to find the train
			trace_t trainTrace;
			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -38 ), 
				MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace );

			if ( trainTrace.fraction != 1.0 && trainTrace.m_pEnt )
				pTrain = dynamic_cast<CFuncTrackTrain*>( trainTrace.m_pEnt );


            // No controllable train found.
			if ( !pTrain || !pTrain->OnControls( this ) )
			{
                bOnTrain = false;

                trainFlags |= TRAIN_OFF;
			}
		}

        // Turn off the train if you jump, strafe, or the train controls go dead
		if (    !pTrain
            ||  pTrain->HasSpawnFlags( SF_TRACKTRAIN_NOCONTROL )
            ||  !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE)
            ||  !( GetFlags() & FL_ONGROUND )
            ||  (m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT|IN_JUMP) ) )
		{
            bOnTrain = false;

            trainFlags |= TRAIN_OFF;
		}

        if ( pTrain && bOnTrain )
        {
		    SetAbsVelocity( vec3_origin );

		    if ( m_afButtonPressed & IN_FORWARD )
		    {
			    vel = 1;
			    pTrain->Use( this, this, USE_SET, vel );
		    }
		    else if ( m_afButtonPressed & IN_BACK )
		    {
			    vel = -1;
			    pTrain->Use( this, this, USE_SET, vel );
		    }

		    if ( vel )
		    {
			    m_iTrain = TrainSpeed( pTrain->m_flSpeed, pTrain->GetMaxSpeed() );
			    m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		    }
        }
	} 


    if ( bOnTrain != bWasOnTrain )
    {
        if ( bOnTrain )
        {
            m_afPhysicsFlags |= PFLAG_DIROVERRIDE;
            m_iTrain |= TRAIN_ACTIVE;
        }
        else
        {
            m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
            m_iTrain &= ~TRAIN_ACTIVE;
        }

        // Send a usermessage telling them the train state changed.
        m_iTrain |= TRAIN_NEW | trainFlags;
    }


    //
	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
		AddFlag( FL_ONTRAIN );
	else 
		RemoveFlag( FL_ONTRAIN );
}

void CZMPlayer::CheckTrainUpdate()
{
    if ( (m_iTrain & TRAIN_NEW) != 0 )
    {
        // We don't need this.

        //CSingleUserRecipientFilter user( this );
        //user.MakeReliable();

        //// send "Train" update message
        //UserMessageBegin( user, "Train" );
        //    WRITE_BYTE(m_iTrain & 0xF);
        //MessageEnd();

        m_iTrain &= ~TRAIN_NEW;
    }
}

void CZMPlayer::PostThink()
{
    HandleDamagesFromUserCmd();

    BaseClass::PostThink();
    

    // I have yet to see what the point of this is. (originates from HL2MP)
    //
    // This makes players die sooner from doors closing from top to bottom. (ie. compound, crocodile)
    // Most likely also the culprit for getting players stuck on each other's head when the bottom one is crouching.
    //
    // But it was causing prediction errors so fuck it. It's bullshit.

    //if ( GetFlags() & FL_DUCKING )
    //{
    //    SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
    //}


    QAngle angles = GetLocalAngles();
    angles[PITCH] = 0;
    SetLocalAngles( angles );

    m_angEyeAngles = EyeAngles();
    m_pPlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
}

void CZMPlayer::UpdateFlashlight()
{
    if ( FlashlightIsOn() )
    {
        SetFlashlightBattery( GetFlashlightBattery() - gpGlobals->frametime * zm_sv_flashlightdrainrate.GetFloat() );

        if ( GetFlashlightBattery() < 0.0f )
        {
            SetFlashlightBattery( 0.0f );
            FlashlightTurnOff();
        }
    }
    else
    {
        SetFlashlightBattery( GetFlashlightBattery() + gpGlobals->frametime * zm_sv_flashlightrechargerate.GetFloat() );
            
        if ( GetFlashlightBattery() > 100.0f )
        {
            SetFlashlightBattery( 100.0f );
        }
    }
}

void CZMPlayer::CopyWeaponDamage( CZMBaseWeapon* pWeapon, const FireBulletsInfo_t& info )
{
    const CUserCmd* pCmd = GetCurrentCommand();
    Assert( pCmd != nullptr );
    if ( !pCmd )
        return;

    m_ServerWepData.iAmmoType = info.m_iAmmoType;
    m_ServerWepData.flDamage = info.m_iPlayerDamage;
    m_ServerWepData.iLastFireCommandNumber = pCmd->command_number;
    m_ServerWepData.hWeapon.Set( pWeapon );
    m_ServerWepData.vecShootPos = info.m_vecSrc;
    m_ServerWepData.bIsMelee = false;
}

void CZMPlayer::CopyMeleeDamage( CZMBaseWeapon* pWeapon, const Vector& vecSrc, float flDamage )
{
    const CUserCmd* pCmd = GetCurrentCommand();
    Assert( pCmd != nullptr );
    if ( !pCmd )
        return;

    m_ServerWepData.iAmmoType = -1;
    m_ServerWepData.flDamage = flDamage;
    m_ServerWepData.iLastFireCommandNumber = pCmd->command_number;
    m_ServerWepData.hWeapon.Set( pWeapon );
    m_ServerWepData.vecShootPos = vecSrc;
    m_ServerWepData.bIsMelee = true;
}

void CZMPlayer::HandleDamagesFromUserCmd()
{
    const CUserCmd* pCmd = GetCurrentUserCommand();

    if ( !pCmd || !pCmd->zmHitData.Count() )
        return;


    g_ZMUserCmdSystem.ApplyDamage( this, m_ServerWepData, pCmd->zmHitData );


    m_ServerWepData.Reset();
}

//
// Player's Think function. Before PreThink/PostThink and all that.
//
void CZMPlayer::PhysicsSimulate()
{
    bool bDoSimulate = m_nSimulationTick != gpGlobals->tickcount;

    BaseClass::PhysicsSimulate();

    if ( bDoSimulate )
    {
        //
        // Put everything here that is time sensitive.
        // Usercmd rate can't be trusted. There maybe holes (as big as 0.1s)
        // between usercmds depending on player's ping and loss.
        // This can fuck up some logic.
        //


        // This fixes continuous use buttons (momentary_rot_button) changing directions.
        // Their logic is run every 0.1s, so we need call Use within that time frame.
        PlayerUse();
    }
}

void CZMPlayer::PlayerDeathThink()
{
    if( !IsObserver() )
    {
        BaseClass::PlayerDeathThink();
    }
}

class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	virtual int ShouldMoveTo( IPhysicsObject* pObject, const Vector& position ) OVERRIDE
	{
		auto* pPlayer = ToZMPlayer( (CBaseEntity*)pObject->GetGameData() );
		if ( pPlayer )
		{
			if ( pPlayer->TouchedPhysics() )
			{
				return 0;
			}
		}
		return 1;
	}
};

static CPhysicsPlayerCallback playerCallback;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMPlayer::InitVCollision( const Vector& vecAbsOrigin, const Vector& vecAbsVelocity )
{
	BaseClass::InitVCollision( vecAbsOrigin, vecAbsVelocity );

	// Setup the HL2 specific callback.
	IPhysicsPlayerController *pPlayerController = GetPhysicsController();
	if ( pPlayerController )
	{
		pPlayerController->SetEventHandler( &playerCallback );
	}
}

void CZMPlayer::SetupVisibility( CBaseEntity* pViewEntity, unsigned char* pvs, int pvssize )
{
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );
}

void CZMPlayer::PushAway( const Vector& pos, float force )
{
    Vector fwd = GetAbsOrigin() - pos;
    fwd.z = 0.0f;

    VectorNormalize( fwd );


    Vector vel = force * fwd;
    vel.z = GetAbsVelocity().z; // Make sure we have our z-velocity the same in case we need to apply fall damage or something.

    SetAbsVelocity( vel );
}

bool CZMPlayer::ClientCommand( const CCommand &args )
{
    return CBasePlayer::ClientCommand( args );
}

void CZMPlayer::ChangeTeam( int iTeam )
{
    int oldteam = GetTeamNumber();

    // Change the team silently...
    CBasePlayer::ChangeTeam( iTeam, true, true );


    if ( oldteam != ZMTEAM_UNASSIGNED && ShouldSpawn() )
    {
        Spawn();
    }
    else
    {
        SetTeamSpecificProps();
    }


    CZMRules* pRules = ZMRules();
    Assert( pRules );
    
    // If we changed teams in the middle of the round (late-joining, etc.) send objectives. 
    if ( iTeam != oldteam && !pRules->IsInRoundEnd() && !(oldteam == ZMTEAM_HUMAN && iTeam != ZMTEAM_SPECTATOR) )
    {
        if ( pRules->GetObjManager() )
        {
            pRules->GetObjManager()->ChangeDisplay( this );
        }
    }

    // See if we need to restart the round.
    // HACK: Don't check for it if our new team isn't spectator.
    // This makes sure we can test stuff while being the only survivor.
    if (!pRules->IsInRoundEnd()
    &&  oldteam > ZMTEAM_SPECTATOR
    &&  iTeam <= ZMTEAM_SPECTATOR)
    {
        CTeam* team = GetGlobalTeam( oldteam );
        if ( team && team->GetNumPlayers() <= 0 )
        {
            pRules->EndRound( ZMROUND_GAMEBEGIN );
        }
    }
}

bool CZMPlayer::ShouldSpawn()
{
    int team = GetTeamNumber();

    if ( team == ZMTEAM_SPECTATOR )
        return false;


    return true;
}

ConVar zm_mp_forcecamera( "zm_mp_forcecamera", "0", FCVAR_NOTIFY | FCVAR_ARCHIVE, "Are spectators disallowed to spectate the ZM?" );

bool CZMPlayer::IsValidObserverTarget( CBaseEntity* pEnt )
{
    if ( !pEnt ) return false;

    // ZMRTODO: Allow zombie spectating, etc.
    if ( !pEnt->IsPlayer() )
    {
        if ( pEnt->IsBaseZombie() )
            return true;

        return false;
    }


    CZMPlayer* pPlayer = ToZMPlayer( pEnt );

    if ( !pPlayer )
        return false;

    // Don't spec observers or players who haven't picked a class yet
    if ( pPlayer->IsObserver() )
        return false;

    if( pPlayer == this )
        return false; // We can't observe ourselves.

    if ( pPlayer->m_lifeState == LIFE_RESPAWNABLE ) // target is dead, waiting for respawn
        return false;

    if ( pPlayer->m_lifeState == LIFE_DEAD || pPlayer->m_lifeState == LIFE_DYING )
    {
        if ( (pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
        {
            return false; // allow watching until 3 seconds after death to see death animation
        }
    }
        

    if ( zm_mp_forcecamera.GetBool() )
    {
        if ( pPlayer->IsZM() )
            return false;
    }

    return true;
}

CBaseEntity* CZMPlayer::FindNextObserverTarget( bool bReverse )
{
    // If we were spectating a zombie, find next zombie.
    CZMBaseZombie* pZombie;
    if (GetObserverTarget()
    &&  GetObserverMode() != OBS_MODE_ROAMING
    &&  (pZombie = dynamic_cast<CZMBaseZombie*>( GetObserverTarget() )) != nullptr )
    {
        CZMBaseZombie* pLoop;

        int origin = -1;
        int len = g_ZombieManager.GetNumZombies();
        
        g_ZombieManager.ForEachZombieRet( [ &origin, pZombie ]( int index, CZMBaseZombie* pLoopZombie )
        {
            if ( pZombie == pLoopZombie )
            {
                origin = index;
                return true;
            }

            return false;
        } );

        if ( origin != -1 )
        {
            int iDir = bReverse ? -1 : 1;
            int i = origin;
            while ( true )
            {
                i += iDir;

                if ( i >= len )
                    i = 0;
                else if ( i < 0 )
                    i = len - 1;

                if ( i == origin )
                    break;

                
                pLoop = g_ZombieManager.GetZombieByIndex( i );
                if ( pLoop && pLoop->IsAlive() )
                {
                    return pLoop;
                }
            }
        }
    }


    return BaseClass::FindNextObserverTarget( bReverse );
}

void CZMPlayer::CheckObserverSettings()
{
    // HACK: Fix zombie spectating not transmitting things properly because view offsets are at origin
    if ( GetObserverMode() == OBS_MODE_CHASE )
    {
        CBaseEntity* pTarget = GetObserverTarget();

        if ( pTarget && pTarget->IsBaseZombie() )
        {
            if ( pTarget->GetViewOffset() != GetViewOffset() )
            {
                SetViewOffset( pTarget->GetViewOffset() );
            }
        }
    }

    BaseClass::CheckObserverSettings();
}

void CZMPlayer::SetTeamSpecificProps()
{
    // To shut up the asserts...
    // These states aren't even used, except for going into observer mode.
    State_Transition( ZMSTATE_ACTIVE );


    RemoveFlag( FL_NOTARGET );


    switch ( GetTeamNumber() )
    {
    case ZMTEAM_ZM :
    {
        m_Local.m_iHideHUD |= HIDEHUD_HEALTH;

        // This is for maps that use the OnBreak trick to name the ZM. This does not work in ZMR anymore.
        const char* targetname = zm_sv_comp_zmtargetname.GetString();
        if ( targetname && *targetname )
        {
            SetName( AllocPooledString( targetname ) );
        }

    }
    case ZMTEAM_SPECTATOR :
        RemoveAllItems( true );

        // HACK: UpdatePlayerSound will make NPCs hear our "footsteps" even as a ZM when we're "on the ground".
        RemoveFlag( FL_ONGROUND );
        SetGroundEntity( nullptr ); // ZM's ground entity doesn't get updated, must reset it here or the view can act up

        AddFlag( FL_NOTARGET );

        if ( IsZM() )
        {
            m_takedamage = DAMAGE_NO;

            SetMoveType( MOVETYPE_NOCLIP );
            SetCollisionGroup( COLLISION_GROUP_DEBRIS );
            AddSolidFlags( FSOLID_NOT_SOLID );

            // Client will simply not render the ZM.
            // This allows the ZM movement to be interpolated.
            //AddEffects( EF_NODRAW );

            // I'm not sure if the vphysics object does anything in ZM's case, but let's see what happens with this.
            VPhysicsDestroyObject();
        }
        else
        {
            // Apparently this sets the observer physics flag.
            State_Transition( ZMSTATE_OBSERVER_MODE );
        }


        if ( FlashlightIsOn() )
            FlashlightTurnOff();

        break;
    default : break;
    }
}


void CZMPlayer::PickDefaultSpawnTeam()
{
    if ( GetTeamNumber() == ZMTEAM_UNASSIGNED )
    {
        ChangeTeam( ZMTEAM_SPECTATOR );
    }
}

// Override this function to stop AmmoDenied usermessage spam.
int CZMPlayer::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound )
{
    // Don't try to give the player invalid ammo indices.
    if ( nAmmoIndex < 0 || nAmmoIndex >= MAX_AMMO_SLOTS )
        return 0;


    bool bCheckAutoSwitch = false;
    if ( !HasAnyAmmoOfType( nAmmoIndex ) )
    {
        bCheckAutoSwitch = true;
    }


    // Game rules say I can't have any more of this ammo type.
    if ( !g_pGameRules->CanHaveAmmo( this, nAmmoIndex ) )
    {
        return 0;
    }


    int nRoom = GetAmmoRoom( nAmmoIndex ) - GetTotalAmmoAmount( nAmmoIndex );
    int nAdd = MIN( nCount, nRoom );
    nAdd = MAX( 0, nAdd );


    if ( nAdd )
    {
        // Ammo pickup sound
        if ( !bSuppressSound )
        {
            CRecipientFilter filter;
            GetMyRecipientFilter( filter );

            CBaseEntity::EmitSound( filter, entindex(), "ZMPlayer.PickupAmmo" );
        }

        m_iAmmo.Set( nAmmoIndex, m_iAmmo[nAmmoIndex] + nAdd );
    }
    



    // We've been denied the pickup, display a hud icon to show that.
    // Make sure we don't send this usermessage multiple times a frame.
    if ( nCount > 0 && nAdd == 0 && m_vAmmoDenied.Find( nAmmoIndex ) == m_vAmmoDenied.InvalidIndex() )
    {
        CSingleUserRecipientFilter user( this );
        user.MakeReliable();
        UserMessageBegin( user, "AmmoDenied" );
            WRITE_SHORT( nAmmoIndex );
        MessageEnd();


        m_vAmmoDenied.AddToTail( nAmmoIndex );
    }

    //
    // If I was dry on ammo for my best weapon and justed picked up ammo for it,
    // autoswitch to my best weapon now.
    //
    if ( bCheckAutoSwitch )
    {
        CBaseCombatWeapon* pWeapon = g_pGameRules->GetNextBestWeapon( this, GetActiveWeapon() );

        if ( pWeapon && pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
        {
            SwitchToNextBestWeapon( GetActiveWeapon() );
        }
    }

    return nAdd;
}

void CZMPlayer::GiveDefaultItems()
{
    RemoveAllItems( false );


    if ( !IsSuitEquipped() )
        EquipSuit( false ); // Don't play "effects" (hand showcase anim)
    

    GiveNamedItem( "weapon_zm_fistscarry" );


    CZMRules* pRules = ZMRules();

    if ( pRules )
    {
        CZMEntLoadout* pLoadout = pRules->GetLoadoutEnt();

        if ( pLoadout )
        {
            pLoadout->DistributeToPlayer( this );
        }
    }


    // Change weapon to best we have.
    CBaseCombatWeapon* pWep = GetWeaponOfHighestSlot();
    
    if ( pWep )
    {
        Weapon_Switch( pWep );
    }
}

void CZMPlayer::ImpulseCommands()
{
    int iImpulse = GetImpulse();
    switch ( iImpulse )
    {
    case 201 : // Spraying
    {
        // Don't allow ZM to spray...
        if ( IsZM() ) return;

        break;
    }
    default : break;
    }

    BaseClass::ImpulseCommands();
}

void CZMPlayer::CheatImpulseCommands( int iImpulse )
{
    switch ( iImpulse )
    {
    case 101 :
    {
        if ( !sv_cheats->GetBool() ) return;

        
        GiveNamedItem( "weapon_zm_pistol" );
        GiveNamedItem( "weapon_zm_revolver" );
        GiveNamedItem( "weapon_zm_improvised" );
        GiveNamedItem( "weapon_zm_sledge" );
        GiveNamedItem( "weapon_zm_shotgun" );
        GiveNamedItem( "weapon_zm_rifle" );
        GiveNamedItem( "weapon_zm_mac10" );
        GiveNamedItem( "weapon_zm_molotov" );

        CBasePlayer::GiveAmmo( 80, "Pistol" );
        CBasePlayer::GiveAmmo( 36, "Revolver" );
        CBasePlayer::GiveAmmo( 24, "Buckshot" );
        CBasePlayer::GiveAmmo( 11, "357" );
        CBasePlayer::GiveAmmo( 60, "SMG1" );


        break;
    }
    // HL2 class has some stuff we don't want or need.
    default : CBasePlayer::CheatImpulseCommands( iImpulse ); break;
    }
}

void CZMPlayer::EquipSuit( bool bPlayEffects )
{
    // Never play the effect.
    CBasePlayer::EquipSuit( false );
}

void CZMPlayer::RemoveAllItems( bool removeSuit )
{
    BaseClass::RemoveAllItems( removeSuit );

    // ZMRTODO: See if this has any side-effects.
    // HACK: To stop ZM having a viewmodel. Just hide our viewmodel.
    if ( GetViewModel( VMINDEX_WEP ) )
    {
        GetViewModel()->AddEffects( EF_NODRAW );
    }
    if ( GetViewModel( VMINDEX_HANDS ) )
    {
        GetViewModel( VMINDEX_HANDS )->AddEffects( EF_NODRAW );
    }
}

int CZMPlayer::FlashlightIsOn()
{
    return IsFlashlightOn();
}

void CZMPlayer::FlashlightTurnOn()
{
    if ( IsHuman() && IsAlive() && GetFlashlightBattery() > 0.0f )
    {
        if ( !IsFlashlightOn() )
        {
            EmitSound( "ZMPlayer.FlashlightOn" );
        }

        AddEffects( EF_DIMLIGHT );
    }
}

void CZMPlayer::FlashlightTurnOff()
{
    if ( IsHuman() && IsAlive() && IsFlashlightOn() )
    {
        EmitSound( "ZMPlayer.FlashlightOff" );
    }

    RemoveEffects( EF_DIMLIGHT );
}

void CZMPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
    // CBasePlayer may still call this.
}

bool CZMPlayer::SetPlayerModel()
{
    bool changed = false;


    const char* pszFallback = ZMGetPlayerModels()->GetRandomPlayerModel()->GetModelName();

    const char* szModelName = nullptr;
    const char* pszCurrentModelName = modelinfo->GetModelName( GetModel() );


    szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );


    int modelIndexCurrent = modelinfo->GetModelIndex( pszCurrentModelName );
    int modelIndex = modelinfo->GetModelIndex( szModelName );


    // The model player wanted is invalid?
    if ( modelIndex == -1 || ZMGetPlayerModels()->GetPlayerModelData( szModelName ) == nullptr )
    {
        // The current model is also fucked, fallback.
        if (modelIndexCurrent == -1
        ||  ZMGetPlayerModels()->GetPlayerModelData( pszCurrentModelName ) == nullptr)
        {
            pszCurrentModelName = pszFallback;
            modelIndexCurrent = -1;
        }

        // Update the cvar on the client's side to make sure they have it right also.
        engine->ClientCommand( edict(), UTIL_VarArgs( "cl_playermodel %s", pszCurrentModelName ) );

        modelIndex = -1;
    }

    if ( modelIndex == -1 )
    {
        szModelName = modelIndexCurrent != -1 ? pszCurrentModelName : pszFallback;
    }

    if ( modelIndexCurrent == -1 || modelIndex != modelIndexCurrent )
    {
        SetModel( szModelName );
        changed = true;
    }


    if ( changed )
    {
        SetHandsData( ZMGetPlayerModels()->GetPlayerModelData( szModelName ) );
    }

    m_flNextModelChangeTime = gpGlobals->curtime + zm_sv_modelchangedelay.GetFloat();

    return changed;
}

void CZMPlayer::UpdatePlayerFOV()
{
    const char* szFov = engine->GetClientConVarValue( entindex(), "zm_cl_fov" );


    int newFov = szFov ? atoi( szFov ) : 0;

    newFov = clamp( newFov > 0 ? newFov : g_pGameRules->DefaultFOV(), ZM_MIN_FOV, MAX_FOV );


    if ( newFov != GetDefaultFOV() )
    {
        SetDefaultFOV( newFov );
    }
}

void CZMPlayer::UpdatePlayerInterpNPC()
{
    const char* szValue = engine->GetClientConVarValue( entindex(), "cl_interp_npcs" );
    float value = szValue ? atof( szValue ) : 0.0f;

    m_flInterpNPCTime = clamp( value, 0.0f, 0.5f );
}

void CZMPlayer::SafelyClampZMValue( float& value, float min, float max )
{
    // We'll have to make sure the client isn't trying to pass infinity/nan
    // That's no good.
    bool valid = value <= max && value >= min;
    if ( valid )
        return;


    if ( value > max )
    {
        value = max;
    }
    else
    {
        value = min;
    }
}

void CZMPlayer::UpdatePlayerZMVars()
{
    const char* szValue;

    szValue = engine->GetClientConVarValue( entindex(), "zm_cl_zmmovespeed" );
    m_flZMMoveSpeed = szValue ? atof( szValue ) : 0.0f;
    SafelyClampZMValue( m_flZMMoveSpeed, 100.0f, 10000.0f );

    szValue = engine->GetClientConVarValue( entindex(), "zm_cl_zmmoveaccelerate" );
    m_flZMMoveAccel = szValue ? atof( szValue ) : 0.0f;
    SafelyClampZMValue( m_flZMMoveAccel, 1.0f, 50.0f );

    szValue = engine->GetClientConVarValue( entindex(), "zm_cl_zmmovedecelerate" );
    m_flZMMoveDecel = szValue ? atof( szValue ) : 0.0f;
    SafelyClampZMValue( m_flZMMoveDecel, 1.0f, 50.0f );
}

void CZMPlayer::InitialSpawn()
{
    BaseClass::InitialSpawn();

    // During initial spawn, players should already be authenticated.
    GetZMRejoinSystem()->OnPlayerJoin( this );
}

void CZMPlayer::InitZMFog()
{
    auto* pFog = ZMRules()->GetZMFogController();
    if ( !pFog )
        return;

    if ( pFog->IsGameCreated() && !CZMEntFogController::IsEnabled() )
        return;


    m_Local.m_PlayerFog.m_hCtrl.Set( pFog );


    // Skybox data is separate.
    {
        m_Local.m_skybox3d.fog.enable = pFog->m_fog.enable;

        m_Local.m_skybox3d.fog.colorPrimary = m_Local.m_skybox3d.fog.colorSecondary = pFog->m_fog.colorPrimary;

        m_Local.m_skybox3d.fog.start = pFog->m_fog.start;
        m_Local.m_skybox3d.fog.end = pFog->m_fog.end;

        m_Local.m_skybox3d.fog.maxdensity = pFog->m_fog.maxdensity;
        m_Local.m_skybox3d.fog.farz = pFog->m_flSkyboxFarZ;

        m_Local.m_skybox3d.fog.lerptime = -1.0f;
    }
}

void CZMPlayer::Spawn()
{
    m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;


    PickDefaultSpawnTeam();
    
    // Must set player model before calling base class spawn...
    SetPlayerModel();
    // Collision group must be set before base class spawn.
    // Spawnpoint look-up depend on the fact that player's are solid which is done before setting player's collision group...
    SetCollisionGroup( COLLISION_GROUP_PLAYER );

    CBasePlayer::Spawn();


    if ( GetTeamNumber() == ZMTEAM_ZM )
    {
        InitZMFog();
    }
    else
    {
        // Make sure sky camera stuff gets updated on spawn.
        m_Local.m_pOldSkyCamera = nullptr;
    }

    if ( !m_Local.m_PlayerFog.m_hCtrl.Get() )
    {
        m_Local.m_skybox3d.fog.enable = false;
    }


    SetTeamSpecificProps();


    if ( !IsObserver() )
    {
        pl.deadflag = false;


        if ( IsHuman() )
        {
            RemoveSolidFlags( FSOLID_NOT_SOLID );

            GiveDefaultItems();

            SetMaxSpeed( ZM_WALK_SPEED );
        }

        RemoveEffects( EF_NODRAW );
    }


    m_nRenderFX = kRenderNormal;


    if ( IsHuman() )
        AddFlag( FL_ONGROUND );
    else
        RemoveFlag( FL_ONGROUND );

    /*if ( HL2MPRules()->IsIntermission() )
    {
        AddFlag( FL_FROZEN );
    }
    else
    {
        RemoveFlag( FL_FROZEN );
    }*/

    //m_bReady = false;


    // Set in BasePlayer
    //m_Local.m_bDucked = false;
    //SetPlayerUnderwater( false );


    // Take 4x more damage when impacted by physics. (Same as HL2DM)
    m_impactEnergyScale = 4.0f;


    // Reset our base velocity so we don't go flying. RIP Muob
    SetBaseVelocity( vec3_origin );


    // Reset activity. Makes sure we don't get insta-punished when spawning after spectating somebody, etc.
    m_flLastActivity = gpGlobals->curtime;

    SetFlashlightBattery( 100.0f );


    // Don't display crosshair if the map is a background.
    if ( gpGlobals->eLoadType == MapLoad_Background )
    {
        ShowCrosshair( false );
    }


    DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
}

void CZMPlayer::CreateCorpse()
{
    Assert( 0 );
}

bool CZMPlayer::BecomeRagdollOnClient( const Vector& force )
{
    return true;
}

void CZMPlayer::CreateRagdollEntity()
{
    if ( m_hRagdoll.Get() )
    {
        UTIL_RemoveImmediate( m_hRagdoll.Get() );
        m_hRagdoll.Set( nullptr );
    }

    // If we already have a ragdoll, don't make another one.
    CZMRagdoll* pRagdoll = dynamic_cast<CZMRagdoll*>( m_hRagdoll.Get() );
    
    if ( !pRagdoll )
    {
        // create a new one
        pRagdoll = dynamic_cast<CZMRagdoll*>( CreateEntityByName( "zm_ragdoll" ) );
    }

    if ( pRagdoll )
    {
        pRagdoll->m_hPlayer = this;
        pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
        pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
        pRagdoll->m_nModelIndex = m_nModelIndex;
        pRagdoll->m_nForceBone = m_nForceBone;
        pRagdoll->m_vecForce = m_vecTotalBulletForce;
        pRagdoll->SetAbsOrigin( GetAbsOrigin() );

        // Copy the effect over.
        CEntityFlame* pFlame = dynamic_cast<CEntityFlame*>( GetEffectEntity() );
        if ( pFlame )
        {
            pFlame->AttachToEntity( pRagdoll );
            pRagdoll->SetEffectEntity( pFlame );

            SetEffectEntity( nullptr );
        }
    }

    // ragdolls will be removed on round restart automatically
    m_hRagdoll.Set( pRagdoll );
}

extern ConVar sv_maxunlag;

bool CZMPlayer::WantsLagCompensationOnNPC( const CZMBaseZombie* pZombie, const CUserCmd* pCmd, const CBitVec<MAX_EDICTS>* pEntityTransmitBits ) const
{
    // If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
    if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pZombie->entindex() ) )
        return false;

#define NPC_MAXSPEED        100.0f
    const Vector &vMyOrigin = GetAbsOrigin();
    const Vector &vHisOrigin = pZombie->GetAbsOrigin();

    // get max distance player could have moved within max lag compensation time, 
    // multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
    float maxDistance = 1.5f * NPC_MAXSPEED * sv_maxunlag.GetFloat();

    // If the player is within this distance, lag compensate them in case they're running past us.
    if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
        return true;

    // If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
    Vector vForward;
    AngleVectors( pCmd->viewangles, &vForward );
    
    Vector vDiff = vHisOrigin - vMyOrigin;
    VectorNormalize( vDiff );
    
    float flCosAngle = DOT_45DEGREE;
    if ( vForward.Dot( vDiff ) < flCosAngle )
        return false;

    return true;
}

void CZMPlayer::Event_Killed( const CTakeDamageInfo &info )
{
    //update damage info with our accumulated physics force
    CTakeDamageInfo subinfo = info;
    subinfo.SetDamageForce( m_vecTotalBulletForce );


    // Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
    // because we still want to transmit to the clients in our PVS.
    CreateRagdollEntity();

    // We can't be on fire while dead!
    Extinguish();

    CBaseEntity* pEffect = GetEffectEntity();
    if ( pEffect != nullptr )
    {
        UTIL_Remove( pEffect );
        SetEffectEntity( nullptr );
    }


    BaseClass::Event_Killed( subinfo );

    if ( info.GetDamageType() & DMG_DISSOLVE )
    {
        if ( m_hRagdoll )
        {
            m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
        }
    }


    CBaseEntity *pAttacker = info.GetAttacker();

    if ( pAttacker )
    {
        int iScoreToAdd = 1;

        if ( pAttacker == this )
        {
            iScoreToAdd = -1;
        }

        GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
    }


    FlashlightTurnOff();

    m_lifeState = LIFE_DEAD;

    RemoveEffects( EF_NODRAW );	// still draw player body

    // ZMRTODO: Figure out how to call EndTouch for all entities on death.
    StopWaterDeathSounds();
}

extern ConVar friendlyfire;

int CZMPlayer::OnTakeDamage( const CTakeDamageInfo& inputInfo )
{
    // Fix for molotov fire damaging other players.
    if ( !friendlyfire.GetBool() )
    {
        CBaseEntity* pAttacker = inputInfo.GetAttacker();

        if ( pAttacker && pAttacker->GetOwnerEntity() )
        {
            if ( pAttacker->GetOwnerEntity()->GetTeamNumber() >= ZMTEAM_SPECTATOR )
            {
                return 0;
            }
        }
    }

    m_vecTotalBulletForce += inputInfo.GetDamageForce();

    //gamestats->Event_PlayerDamage( this, inputInfo );

    return BaseClass::OnTakeDamage( inputInfo );
}

int CZMPlayer::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
	// Drown
	if( info.GetDamageType() & DMG_DROWN )
	{
		if( m_idrowndmg == m_idrownrestored )
		{
			EmitSound( "Player.DrownStart" );
		}
		else
		{
			EmitSound( "Player.DrownContinue" );
		}
	}

	// Burnt
	if ( info.GetDamageType() & DMG_BURN )
	{
		EmitSound( "HL2Player.BurnPain" );
	}

	// Call the base class implementation
	return BaseClass::OnTakeDamage_Alive( info );
}

void CZMPlayer::CommitSuicide( bool bExplode, bool bForce )
{
    if ( !IsHuman() ) return;

    
    BaseClass::CommitSuicide( bExplode, bForce );
}

void CZMPlayer::CommitSuicide( const Vector &vecForce, bool bExplode, bool bForce )
{
    if ( !IsHuman() ) return;

    
    BaseClass::CommitSuicide( vecForce, bExplode, bForce );
}

void CZMPlayer::DeathSound( const CTakeDamageInfo &info )
{
    if ( m_hRagdoll.Get() && m_hRagdoll.Get()->GetBaseAnimating()->IsDissolving() )
         return;


    const char* szStepSound = "NPC_Citizen.Die";

    const char *pModelName = STRING( GetModelName() );

    CSoundParameters params;
    if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
        return;

    Vector vecOrigin = GetAbsOrigin();
    
    CRecipientFilter filter;
    filter.AddRecipientsByPAS( vecOrigin );

    EmitSound_t ep;
    ep.m_nChannel = params.channel;
    ep.m_pSoundName = params.soundname;
    ep.m_flVolume = params.volume;
    ep.m_SoundLevel = params.soundlevel;
    ep.m_nFlags = 0;
    ep.m_nPitch = params.pitch;
    ep.m_pOrigin = &vecOrigin;

    EmitSound( filter, entindex(), ep );
}

void CZMPlayer::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
    if ( !IsHuman() || !IsAlive() )
        return;

    BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );
}

extern ConVar physcannon_maxmass;

void CZMPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
    if ( !IsHuman() || !IsAlive() ) return;


    PlayerAttemptPickup( this, pObject );
}

//bool CZMPlayer::IsHoldingEntity( CBaseEntity *pEnt )
//{
//    // Ask our carrying weapon if we're holding it
//    CZMWeaponHands* pWeapon = static_cast<CZMWeaponHands*>( Weapon_OwnsThisType( "weapon_zm_fistscarry" ) );
//    return pWeapon ? pWeapon->IsCarryingObject( pEnt ) : false;
//}

float CZMPlayer::GetHeldObjectMass( IPhysicsObject *pHeldObject )
{
    // Ask our carrying weapon
    CZMWeaponHands* pWeapon = static_cast<CZMWeaponHands*>( Weapon_OwnsThisType( "weapon_zm_fistscarry" ) );

    return pWeapon ? pWeapon->GetHeldObjectMass() : 0.0f;
}

void CZMPlayer::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis )
{
	if ( PhysIsInCallback() )
	{
		variant_t value;
		g_EventQueue.AddEvent( this, "ForceDropPhysObjects", value, 0.01f, pOnlyIfHoldingThis, this );
		return;
	}

	// Let go of func_tanks.
	ClearUseEntity();

	// Do the actual dropping.
	PhysCannonForceDrop( GetActiveWeapon(), nullptr );
}

void CZMPlayer::InputForceDropPhysObjects( inputdata_t &data )
{
	ForceDropOfCarriedPhysObjects( data.pActivator );
}

void CZMPlayer::SetHandsModel( const char* model )
{
    if ( !model || !(*model) ) return;

    CBaseViewModel* pVM = GetViewModel( VMINDEX_HANDS );
    if ( !pVM ) return;

    pVM->SetModel( model );
}

void CZMPlayer::SetHandsData( CZMPlayerModelData* pData )
{
    if ( !pData ) return;

    CZMViewModel* pVM = static_cast<CZMViewModel*>( GetViewModel( VMINDEX_HANDS ) );
    if ( !pVM ) return;



    // Set the model
    const char* pszArms = pData->GetArmModel();
    if ( !pszArms || !(*pszArms) )
    {
        pszArms = VMHANDS_FALLBACKMODEL;
    }

    int modelIndex = modelinfo->GetModelIndex( pszArms );
    if ( modelIndex != -1 && pVM->GetModelIndex() != modelIndex )
    {
        pVM->SetModel( pszArms );
    }



    int skin = pData->GetArmSkin();
    if ( skin >= 0 && skin < MAXSTUDIOSKINS )
    {
        pVM->m_nSkin = skin;
    }

    Color clr = pData->GetArmColor();
    if ( clr[0] != 0 || clr[1] != 0 || clr[2] != 0 )
    {
        pVM->SetModelColor2( clr[0] / 255.0f, clr[1] / 255.0f, clr[2] / 255.0f );
    }
}

void CZMPlayer::CreateViewModel( int index )
{
    // We should never create more than the first index.
    Assert( index == 0 );


    if ( GetViewModel( VMINDEX_WEP ) != nullptr )
    {
        if ( GetViewModel( VMINDEX_HANDS ) == nullptr )
        {
            Warning( "Weapon viewmodel exists but hands don't!!\n" );
        }

        return;
    }


    CZMViewModel* vm = static_cast<CZMViewModel*>( CreateEntityByName( "zm_viewmodel" ) );
    
    if ( vm )
    {
        vm->SetAbsOrigin( GetAbsOrigin() );
        vm->SetOwner( this );
        vm->SetIndex( VMINDEX_WEP );
        DispatchSpawn( vm );
        vm->FollowEntity( this, false );
        m_hViewModel.Set( VMINDEX_WEP, vm );


        CZMViewModel* vmhands = static_cast<CZMViewModel*>( CreateEntityByName( "zm_viewmodel" ) );
    
        if ( vmhands )
        {
            vmhands->SetAbsOrigin( GetAbsOrigin() );
            vmhands->SetOwner( this );
            vmhands->SetIndex( VMINDEX_HANDS );
            DispatchSpawn( vmhands );
            vmhands->FollowEntity( vm, true ); // Sets moveparent.
            m_hViewModel.Set( VMINDEX_HANDS, vmhands );
        }
    }

    SetHandsModel( VMHANDS_FALLBACKMODEL );
    SetHandsData( ZMGetPlayerModels()->GetPlayerModelData( STRING( GetModelName() ) ) );
}

bool CZMPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
    // The visibility check was causing problems. (probably due to shitty collision boxes for the weapons...)

    if ( !IsHuman() ) return false;

    // How would this even be possible?
    if ( pWeapon->GetOwner() ) return false;


    if ( !IsAllowedToPickupWeapons() )
        return false;

    
    if ( /*!Weapon_CanUse( pWeapon ) ||*/ !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
    {
        return false;
    }
    
    
    // Apparently, the reason why this wasn't working was because the center was in solid.
    trace_t tr;
    CTraceFilterSkipTwoEntities filter( this, pWeapon, COLLISION_GROUP_NONE );
    UTIL_TraceLine( pWeapon->WorldSpaceCenter(), EyePosition(), MASK_OPAQUE, &filter, &tr );

    if ( tr.fraction != 1.0f && !tr.startsolid ) return false;
    

    if ( Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) ) 
    {
        return false;
    }

    pWeapon->CheckRespawn();

    Weapon_Equip( pWeapon );

    return true;
}

CBaseEntity* CZMPlayer::EntSelectSpawnPoint( void )
{
    const char *pszFallback = "info_player_deathmatch";
    const char *pszSpawn = IsZM() ? "info_player_zombiemaster" : "info_player_survivor";


    CUtlVector<CBaseEntity*> vValidSpots;
    vValidSpots.Purge();



    bool bCheckEnabled = true;
    CBaseEntity* pEnt;
    
    // Find all valid spawn points.
    for ( int i = 0; i < 2; i++ )
    {
        // Loop through our wanted spawnpoint, if it fails, use fallback.
        for ( int j = 0; j < 2; j++ )
        {
            pEnt = nullptr;
            while ( (pEnt = gEntList.FindEntityByClassname( pEnt, pszSpawn )) != nullptr )
            {
                if ( (!bCheckEnabled || static_cast<CZMEntSpawnPoint*>( pEnt )->IsEnabled()) && g_pGameRules->IsSpawnPointValid( pEnt, this ) )
                {
                    if ( vValidSpots.Find( pEnt ) == -1 )
                        vValidSpots.AddToTail( pEnt );
                }
            }

            if ( vValidSpots.Count() )
                break;

            pszSpawn = pszFallback;
        }

        if ( vValidSpots.Count() )
            break;

        // If even that failed, don't check if the spawnpoint is enabled.
        bCheckEnabled = false;
    }

    if ( vValidSpots.Count() )
    {
        return vValidSpots[random->RandomInt( 0, vValidSpots.Count() - 1 )];
    }


    Warning( "Couldn't find a valid spawnpoint for %i, using first fallback...\n", entindex() );


    // Didn't work, now just find any spot.
    pEnt = gEntList.FindEntityByClassname( nullptr, pszSpawn );
    if ( pEnt ) return pEnt;

    pEnt = gEntList.FindEntityByClassname( nullptr, pszFallback );
    if ( pEnt ) return pEnt;

    pEnt = gEntList.FindEntityByClassname( nullptr, "info_player_start" );
    if ( pEnt ) return pEnt;


    Warning( "Map has no spawnpoints! Returning world...\n" );

    return GetContainingEntity( INDEXENT( 0 ) );
}

void CZMPlayer::DeselectAllZombies()
{
    g_ZombieManager.ForEachSelectedZombie( this, []( CZMBaseZombie* pZombie )
    {
        pZombie->SetSelector( 0 );
    } );
}

void CZMPlayer::PlayerUse()
{
    auto* pOldUseEntity = GetUseEntity();

    bool bCanUse = IsHuman() && IsAlive() && GetMoveType() != MOVETYPE_LADDER;

    if ( !bCanUse )
    {
        if ( pOldUseEntity )
        {
            ClearUseEntity();
        }

        m_afPhysicsFlags &= ~PFLAG_USING;
        return;
    }


    bool bPressedUse = (m_afButtonPressed & IN_USE) != 0;
    bool bReleasedUse = (m_afButtonReleased & IN_USE) != 0;
    bool bHoldingUse = (m_nButtons & IN_USE) != 0;

	// Was use pressed or released?
	if ( !bPressedUse && !bReleasedUse && !bHoldingUse )
		return;

	if ( bPressedUse )
	{
		// Currently using a latched entity?
		if ( ClearUseEntity() )
		{
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				auto* pTrain = dynamic_cast<CFuncTrackTrain*>( GetGroundEntity() );
				if ( pTrain && !(m_nButtons & IN_JUMP) && (GetFlags() & FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(this) )
				{
					m_afPhysicsFlags |= PFLAG_DIROVERRIDE;
					m_iTrain = TrainSpeed( pTrain->m_flSpeed, pTrain->GetMaxSpeed() );
					m_iTrain |= TRAIN_NEW;
					EmitSound( "HL2Player.TrainUse" );
					return;
				}
			}
		}
	}

	CBaseEntity* pUseEntity = FindUseEntity();

	bool bUsedEntity = false;

	// Found an object
	if ( pUseEntity )
	{
		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;


		if ( ( bHoldingUse && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( bPressedUse && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

            if ( caps & FCAP_ONOFF_USE )
            {
                pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_ON );
            }
            else
            {
                pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );
            }

			bUsedEntity = true;
		}
		else if ( bReleasedUse && (caps & FCAP_ONOFF_USE) )
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_OFF );

			bUsedEntity = true;
		}
	}
	
    if ( !bUsedEntity && bPressedUse )
	{
		// Signal that we want to play the deny sound, unless the user is +USEing on a ladder!
		// The sound is emitted in ItemPostFrame, since that occurs after GameMovement::ProcessMove which
		// lets the ladder code unset this flag.
		m_bPlayUseDenySound = true;
	}
    else if ( bUsedEntity && bPressedUse )
    {
        EmitSound( "HL2Player.Use" );
    }
}

//
// Just stock ItemPreFrame, but without the PlayerUse.
//
void CZMPlayer::ItemPreFrame()
{
	//PlayerUse();

	auto* pActive = GetActiveWeapon();

	// Allow all the holstered weapons to update
	for ( int i = 0; i < WeaponCount(); ++i )
	{
		auto* pWeapon = GetWeapon( i );

		if ( !pWeapon )
			continue;

		if ( pActive == pWeapon )
			continue;

		pWeapon->ItemHolsterFrame();
	}

    if ( gpGlobals->curtime < m_flNextAttack )
		return;

	if ( !pActive )
		return;

#if defined( CLIENT_DLL )
	// Not predicting this weapon
	if ( !pActive->IsPredicted() )
		return;
#endif

	pActive->ItemPreFrame();
}

void CZMPlayer::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if ( m_bPlayUseDenySound )
	{
		m_bPlayUseDenySound = false;
		EmitSound( "HL2Player.UseDeny" );
	}
}


void CZMPlayer::StartWaterDeathSounds()
{
	CPASAttenuationFilter filter( this );

	if ( !m_sndLeeches )
	{
		m_sndLeeches = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "coast.leech_bites_loop" , ATTN_NORM );
	}

	if ( m_sndLeeches )
	{
		(CSoundEnvelopeController::GetController()).Play( m_sndLeeches, 1.0f, 100 );
	}

	if ( !m_sndWaterSplashes )
	{
		m_sndWaterSplashes = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "coast.leech_water_churn_loop" , ATTN_NORM );
	}

	if ( m_sndWaterSplashes )
	{
		(CSoundEnvelopeController::GetController()).Play( m_sndWaterSplashes, 1.0f, 100 );
	}
}

void CZMPlayer::StopWaterDeathSounds()
{
	if ( m_sndLeeches )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndLeeches, 0.5f, true );
		m_sndLeeches = nullptr;
	}

	if ( m_sndWaterSplashes )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndWaterSplashes, 0.5f, true );
		m_sndWaterSplashes = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Shuts down sounds
//-----------------------------------------------------------------------------
void CZMPlayer::StopLoopingSounds( void )
{
	if ( m_sndLeeches )
	{
		 (CSoundEnvelopeController::GetController()).SoundDestroy( m_sndLeeches );
		 m_sndLeeches = nullptr;
	}

	if ( m_sndWaterSplashes )
	{
		 (CSoundEnvelopeController::GetController()).SoundDestroy( m_sndWaterSplashes );
		 m_sndWaterSplashes = nullptr;
	}

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: Makes a splash when the player transitions between water states
//-----------------------------------------------------------------------------
void CZMPlayer::Splash()
{
	CEffectData data;
	data.m_fFlags = 0;
	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = Vector( 0, 0, 1 );
	data.m_vAngles = QAngle( 0, 0, 0 );
	
	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	float flSpeed = GetAbsVelocity().Length();
	if ( flSpeed < 300.0f )
	{
		data.m_flScale = random->RandomFloat( 10, 12 );
		DispatchEffect( "waterripple", data );
	}
	else
	{
		data.m_flScale = random->RandomFloat( 6, 8 );
		DispatchEffect( "watersplash", data );
	}
}


CZMBaseWeapon* CZMPlayer::GetWeaponOfHighestSlot() const
{
    CZMBaseWeapon* pWep = nullptr;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_LARGE )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_SIDEARM )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_MELEE )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_EQUIPMENT )) != nullptr )
        return pWep;

    // Just default to fists.
    return ToZMBaseWeapon( Weapon_OwnsThisType( "weapon_zm_fistscarry" ) );
}

CZMBaseWeapon* CZMPlayer::GetWeaponOfSlot( int slot ) const
{
    // You can search for multiple slots.
    if ( !(GetWeaponSlotFlags() & slot) )
        return nullptr;


    CZMBaseWeapon* pWep;

    for ( int i = 0; i < MAX_WEAPONS; i++ ) 
    {
        pWep = ToZMBaseWeapon( m_hMyWeapons[i].Get() );

        if ( pWep && pWep->GetSlotFlag() & slot )
        {
            return pWep;
        }
    }

    return nullptr;
}

CZMBaseWeapon* CZMPlayer::GetWeaponOfSlot( const char* szSlotName ) const
{
    if ( Q_stricmp( szSlotName, "sidearm" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_SIDEARM );
    }

    if ( Q_stricmp( szSlotName, "melee" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_MELEE );
    }

    if ( Q_stricmp( szSlotName, "large" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_LARGE );
    }

    if ( Q_stricmp( szSlotName, "equipment" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_EQUIPMENT );
    }

    return nullptr;
}

int CZMPlayer::ShouldTransmit( const CCheckTransmitInfo* pInfo )
{
    // "The most difficult thing in life is to know yourself." - Some guy
    // I searched for a fitting "know yourself" quote to joke about it but I shouldn't have put so much effort into this...
    if ( pInfo->m_pClientEnt == edict() )
    {
        return FL_EDICT_ALWAYS;
    }


    if ( IsEffectActive( EF_NODRAW ) )
        return FL_EDICT_DONTSEND;


    // Don't send observers at all.
    // The player doesn't get switched fast enough to warrant this.
    if ( IsObserver()
    /*&&  (gpGlobals->curtime - m_flDeathTime) > 0.5f
    &&  m_lifeState == LIFE_DEAD && (gpGlobals->curtime - m_flDeathAnimTime) > 0.5f*/ )
    {
        return FL_EDICT_DONTSEND;
    }



    CZMPlayer* pRecipientPlayer = static_cast<CZMPlayer*>( CBaseEntity::Instance( pInfo->m_pClientEnt ) );

    if ( !pRecipientPlayer )
        return FL_EDICT_DONTSEND;


    if ( IsZM() )
    {
        // Don't send us if the recipient is alive, they're not suppose to know about us.
        if ( !pRecipientPlayer->IsObserver() )
        {
            return FL_EDICT_DONTSEND;
        }
    }


    return CBaseEntity::ShouldTransmit( pInfo );
}

void CZMPlayer::State_Transition( ZMPlayerState_t newState )
{
    State_Leave();
    State_Enter( newState );
}


void CZMPlayer::State_Enter( ZMPlayerState_t newState )
{
    m_iPlayerState = newState;
    m_pCurStateInfo = State_LookupInfo( newState );

    // Initialize the new state.
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
        (this->*m_pCurStateInfo->pfnEnterState)();
}


void CZMPlayer::State_Leave()
{
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
    {
        (this->*m_pCurStateInfo->pfnLeaveState)();
    }
}


void CZMPlayer::State_PreThink()
{
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
    {
        (this->*m_pCurStateInfo->pfnPreThink)();
    }
}


CZMPlayerStateInfo* CZMPlayer::State_LookupInfo( ZMPlayerState_t state )
{
    // This table MUST match the 
    static CZMPlayerStateInfo playerStateInfos[] =
    {
        { ZMSTATE_ACTIVE,			"STATE_ACTIVE",			&CZMPlayer::State_Enter_ACTIVE, NULL, &CZMPlayer::State_PreThink_ACTIVE },
        { ZMSTATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CZMPlayer::State_Enter_OBSERVER_MODE,	NULL, &CZMPlayer::State_PreThink_OBSERVER_MODE }
    };

    for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
    {
        if ( playerStateInfos[i].m_iPlayerState == state )
            return &playerStateInfos[i];
    }

    return NULL;
}

bool CZMPlayer::StartObserverMode( int mode )
{
    //we only want to go into observer mode if the player asked to, not on a death timeout
    if ( m_bEnterObserver == true )
    {
        VPhysicsDestroyObject();
        return BaseClass::StartObserverMode( mode );
    }
    return false;
}

void CZMPlayer::StopObserverMode()
{
    m_bEnterObserver = false;
    BaseClass::StopObserverMode();
}

void CZMPlayer::State_Enter_OBSERVER_MODE()
{
    int observerMode = m_iObserverLastMode;
    if ( IsNetClient() )
    {
        const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
        if ( pIdealMode )
        {
            observerMode = atoi( pIdealMode );
            if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
            {
                observerMode = m_iObserverLastMode;
            }
        }
    }
    m_bEnterObserver = true;
    StartObserverMode( observerMode );
}

void CZMPlayer::State_PreThink_OBSERVER_MODE()
{
    // Make sure nobody has changed any of our state.
    //	Assert( GetMoveType() == MOVETYPE_FLY );
    Assert( m_takedamage == DAMAGE_NO );
    Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
    //	Assert( IsEffectActive( EF_NODRAW ) );

    // Must be dead.
    Assert( m_lifeState == LIFE_DEAD );
    Assert( pl.deadflag );
}


void CZMPlayer::State_Enter_ACTIVE()
{
    SetMoveType( MOVETYPE_WALK );
    
    // md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
    // is false, then they'll be spectating but blocking live players from moving.
    // RemoveSolidFlags( FSOLID_NOT_SOLID );
    
    m_Local.m_iHideHUD = 0;
}


void CZMPlayer::State_PreThink_ACTIVE()
{
    //we don't really need to do anything here. 
    //This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death


    UpdateLastKnownArea();
}

void CZMPlayer::UpdateClientData()
{
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = GetLocalOrigin();
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		damageOrigin = m_DmgOrigin;

		// only send down damage type that have hud art
		int iShowHudDamage = g_pGameRules->Damage_GetShowOnHud();
		int visibleDamageBits = m_bitsDamageType & iShowHudDamage;

		m_DmgTake = clamp( m_DmgTake, 0, 255 );
		m_DmgSave = clamp( m_DmgSave, 0, 255 );

		// If we're poisoned, but it wasn't this frame, don't send the indicator
		// Without this check, any damage that occured to the player while they were
		// recovering from a poison bite would register as poisonous as well and flash
		// the whole screen! -- jdw
		if ( visibleDamageBits & DMG_POISON )
		{
			float flLastPoisonedDelta = gpGlobals->curtime - m_tbdPrev;
			if ( flLastPoisonedDelta > 0.1f )
			{
				visibleDamageBits &= ~DMG_POISON;
			}
		}

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( m_DmgSave );
			WRITE_BYTE( m_DmgTake );
			WRITE_LONG( visibleDamageBits );
			WRITE_FLOAT( damageOrigin.x );	//BUG: Should be fixed point (to hud) not floats
			WRITE_FLOAT( damageOrigin.y );	//BUG: However, the HUD does _not_ implement bitfield messages (yet)
			WRITE_FLOAT( damageOrigin.z );	//BUG: We use WRITE_VEC3COORD for everything else
		MessageEnd();
	
		m_DmgTake = 0;
		m_DmgSave = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= iTimeBasedDamage;
	}

	BaseClass::UpdateClientData();
}

void CZMPlayer::ExitLadder()
{
	if ( GetMoveType() == MOVETYPE_LADDER )
    {
	    SetMoveType( MOVETYPE_WALK );
	    SetMoveCollide( MOVECOLLIDE_DEFAULT );
    }

	// Remove from ladder
	m_ZMLocal.m_hLadder.Set( nullptr );
}

surfacedata_t* CZMPlayer::GetLadderSurface( const Vector& origin )
{
	extern const char* FuncLadder_GetSurfaceprops( CBaseEntity* pLadderEntity );

	CBaseEntity* pLadder = m_ZMLocal.m_hLadder.Get();
	if ( pLadder )
	{
		const char* pszSurfaceprops = FuncLadder_GetSurfaceprops(pLadder);
		// get ladder material from func_ladder
		return physprops->GetSurfaceData( physprops->GetSurfaceIndex( pszSurfaceprops ) );

	}
	return BaseClass::GetLadderSurface(origin);
}

int CZMPlayer::GetZMCommandInterruptFlags() const
{
    const char* val = engine->GetClientConVarValue( entindex(), "zm_cl_zmunitcommandinterrupt" );

    if ( !val || !(*val) )
        return 0;

    return atoi( val );
}

void CZMPlayer::GetMyRecipientFilter( CRecipientFilter& filter ) const
{
    filter.AddRecipient( this );

    // Get all spectators spectating me.
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );
        if (pPlayer
            &&  pPlayer->IsObserver()
            &&  pPlayer->GetObserverMode() == OBS_MODE_IN_EYE
            &&  pPlayer->GetObserverTarget() == this)
        {
            filter.AddRecipient( pPlayer );
        }
    }
}
