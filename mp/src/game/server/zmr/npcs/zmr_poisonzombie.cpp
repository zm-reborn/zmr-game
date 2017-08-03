//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A hideous, putrescent, pus-filled undead carcass atop which a vile
//			nest of filthy poisonous headcrabs lurks.
//
//			Anyway, this guy has two range attacks: at short range, headcrabs
//			will leap from the nest to attack. At long range he will wrench a
//			headcrab from his back to throw it at his enemy.
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "game.h"
#include "npc_headcrab.h"
#include "npcevent.h"
#include "entitylist.h"
#include "ai_task.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "npc_BaseZombie.h"


#include "zmr/zmr_gamerules.h"
#include "zmr_zombiebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BREATH_VOL_MAX  0.6

//
// Controls how soon he throws the first headcrab after seeing his enemy (also when the first headcrab leaps off)
//
#define ZOMBIE_THROW_FIRST_MIN_DELAY	1	// min seconds before first crab throw
#define ZOMBIE_THROW_FIRST_MAX_DELAY	2	// max seconds before first crab throw

//
// Controls how often he throws headcrabs (also how often headcrabs leap off)
//
#define ZOMBIE_THROW_MIN_DELAY	4			// min seconds between crab throws
#define ZOMBIE_THROW_MAX_DELAY	10			// max seconds between crab throws

//
// Ranges for throwing headcrabs.
//
#define ZOMBIE_THROW_RANGE_MIN	250
#define ZOMBIE_THROW_RANGE_MAX	800
#define ZOMBIE_THROW_CONE		0.6

//
// Ranges for headcrabs leaping off.
//
#define ZOMBIE_HC_LEAP_RANGE_MIN	12
#define ZOMBIE_HC_LEAP_RANGE_MAX	256
#define ZOMBIE_HC_LEAP_CONE		0.6


#define ZOMBIE_BODYGROUP_NEST_BASE		2	// First nest crab, +2 more
#define ZOMBIE_BODYGROUP_THROW			5	// The crab in our hand for throwing

#define ZOMBIE_ENEMY_BREATHE_DIST		300	// How close we must be to our enemy before we start breathing hard.


envelopePoint_t envPoisonZombieMoanVolumeFast[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};


//
// Turns the breathing off for a second, then back on.
//
envelopePoint_t envPoisonZombieBreatheVolumeOffShort[] =
{
	{	0.0f, 0.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		2.0f, 2.0f,
	},
	{	BREATH_VOL_MAX, BREATH_VOL_MAX,
		1.0f, 1.0f,
	},
};


//
// Custom schedules.
//
enum
{
	SCHED_ZOMBIE_POISON_RANGE_ATTACK2 = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_POISON_RANGE_ATTACK1,
};


//-----------------------------------------------------------------------------
// The maximum number of headcrabs we can have riding on our back.
// NOTE: If you change this value you must also change the lookup table in Spawn!
//-----------------------------------------------------------------------------
#define MAX_CRABS	3	

int AE_ZOMBIE_POISON_THROW_WARN_SOUND;
int AE_ZOMBIE_POISON_PICKUP_CRAB;
int AE_ZOMBIE_POISON_THROW_SOUND;
int AE_ZOMBIE_POISON_THROW_CRAB;
int AE_ZOMBIE_POISON_SPIT;

//-----------------------------------------------------------------------------
// The model we use for our legs when we get blowed up.
//-----------------------------------------------------------------------------
static const char *s_szLegsModel = "models/zombie/classic_legs.mdl";


//-----------------------------------------------------------------------------
// The classname of the headcrab that jumps off of this kind of zombie.
//-----------------------------------------------------------------------------
static const char *s_szHeadcrabClassname = "npc_headcrab_poison";
static const char *s_szHeadcrabModel = "models/headcrabblack.mdl";

static const char *pMoanSounds[] =
{
	"NPC_PoisonZombie.Moan1",
};

//-----------------------------------------------------------------------------
// Skill settings.
//-----------------------------------------------------------------------------
extern ConVar sk_zombie_poison_health;
extern ConVar sk_zombie_poison_dmg_spit;

class CNPC_PoisonZombie : public CAI_BlendingHost<CZMBaseZombie>
{
	DECLARE_CLASS( CNPC_PoisonZombie, CAI_BlendingHost<CZMBaseZombie> );

public:
    CNPC_PoisonZombie::CNPC_PoisonZombie()
    {
        SetZombieClass( ZMCLASS_HULK );
        CZMRules::IncPopCount( GetZombieClass() );
    }
	//
	// CBaseZombie implemenation.
	//
	virtual Vector HeadTarget( const Vector &posSrc );

	//
	// CAI_BaseNPC implementation.
	//
	virtual float MaxYawSpeed( void );

	//virtual int RangeAttack1Conditions( float flDot, float flDist );
	//virtual int RangeAttack2Conditions( float flDot, float flDist );

	virtual float GetClawAttackRange() const { return 70; }

	virtual void PrescheduleThink( void );
	virtual void BuildScheduleTestBits( void );
	virtual int SelectSchedule( void );
	//virtual int SelectFailSchedule( int nFailedSchedule, int nFailedTask, AI_TaskFailureCode_t eTaskFailCode );
	virtual int TranslateSchedule( int scheduleType );

	virtual bool ShouldPlayIdleSound( void );

	//
	// CBaseAnimating implementation.
	//
	virtual void HandleAnimEvent( animevent_t *pEvent );

	//
	// CBaseEntity implementation.
	//
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void SetZombieModel( void );

	virtual void Event_Killed( const CTakeDamageInfo &info );
	//virtual int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void PainSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot ) {};

	virtual void StopLoopingSounds( void );

protected:

	virtual void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );
	virtual bool MustCloseToAttack( void );

	virtual const char *GetMoanSound( int nSoundIndex );

private:

	void BreatheOffShort( void );

	//void EnableCrab( int nCrab, bool bEnable );
	//int RandomThrowCrab( void );
	//void EvacuateNest( bool bExplosion, float flDamage, CBaseEntity *pAttacker );

	CSoundPatch *m_pFastBreathSound;
	CSoundPatch *m_pSlowBreathSound;

	float m_flNextPainSoundTime;

	bool m_bNearEnemy;
};

LINK_ENTITY_TO_CLASS( npc_poisonzombie, CNPC_PoisonZombie );


BEGIN_DATADESC( CNPC_PoisonZombie )

	DEFINE_SOUNDPATCH( m_pFastBreathSound ),
	DEFINE_SOUNDPATCH( m_pSlowBreathSound ),

	DEFINE_FIELD( m_flNextPainSoundTime, FIELD_TIME ),

	DEFINE_FIELD( m_bNearEnemy, FIELD_BOOLEAN ),

	// NOT serialized:
	//DEFINE_FIELD( m_nThrowCrab, FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::Precache( void )
{
	PrecacheModel( "models/zombie/hulk.mdl" );

	PrecacheScriptSound( "NPC_PoisonZombie.Die" );
	//PrecacheScriptSound( "NPC_PoisonZombie.ThrowWarn" );
	//PrecacheScriptSound( "NPC_PoisonZombie.Throw" );
	PrecacheScriptSound( "NPC_PoisonZombie.Idle" );
	PrecacheScriptSound( "NPC_PoisonZombie.Pain" );
	PrecacheScriptSound( "NPC_PoisonZombie.Alert" );
	PrecacheScriptSound( "NPC_PoisonZombie.FootstepRight" );
	PrecacheScriptSound( "NPC_PoisonZombie.FootstepLeft" );
	PrecacheScriptSound( "NPC_PoisonZombie.Attack" );

	PrecacheScriptSound( "NPC_PoisonZombie.FastBreath" );
	PrecacheScriptSound( "NPC_PoisonZombie.Moan1" );

	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::Spawn( void )
{
	Precache();

	m_fIsTorso = m_fIsHeadless = false;

#ifdef HL2_EPISODIC
	SetBloodColor( BLOOD_COLOR_ZOMBIE );
#else
	SetBloodColor( BLOOD_COLOR_YELLOW );
#endif // HL2_EPISODIC

	m_iHealth = sk_zombie_poison_health.GetFloat();
	m_flFieldOfView = 0.2;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1/* | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK2*/ );

	BaseClass::Spawn();

	CPASAttenuationFilter filter( this, ATTN_IDLE );
	m_pFastBreathSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_ITEM, "NPC_PoisonZombie.FastBreath", ATTN_IDLE );
	ENVELOPE_CONTROLLER.Play( m_pFastBreathSound, 0.0f, 100 );

	CPASAttenuationFilter filter2( this );
	m_pSlowBreathSound = ENVELOPE_CONTROLLER.SoundCreate( filter2, entindex(), CHAN_ITEM, "NPC_PoisonZombie.Moan1", ATTN_NORM );
	ENVELOPE_CONTROLLER.Play( m_pSlowBreathSound, BREATH_VOL_MAX, 100 );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CNPC_PoisonZombie::GetMoanSound( int nSound )
{
	return pMoanSounds[nSound % ARRAYSIZE( pMoanSounds )];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::StopLoopingSounds( void )
{
	ENVELOPE_CONTROLLER.SoundDestroy( m_pFastBreathSound );
	m_pFastBreathSound = NULL;

	ENVELOPE_CONTROLLER.SoundDestroy( m_pSlowBreathSound );
	m_pSlowBreathSound = NULL;

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : info - 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::Event_Killed( const CTakeDamageInfo &info )
{
	if ( !( info.GetDamageType() & ( DMG_BLAST | DMG_ALWAYSGIB) ) ) 
	{
		EmitSound( "NPC_PoisonZombie.Die" );
	}

	//if ( !m_fIsTorso )
	//{
	//	EvacuateNest(info.GetDamageType() == DMG_BLAST, info.GetDamage(), info.GetAttacker() );
	//}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_PoisonZombie::MaxYawSpeed( void )
{
	return BaseClass::MaxYawSpeed();
}

void CNPC_PoisonZombie::SetZombieModel( void )
{
	Hull_t lastHull = GetHullType();

	SetModel( "models/zombie/hulk.mdl" );
	SetHullType( HULL_HUMAN );

	//SetBodygroup( ZOMBIE_BODYGROUP_HEADCRAB, 0 );

	SetHullSizeNormal( true );
	SetDefaultEyeOffset();
	SetActivity( ACT_IDLE );

	// hull changed size, notify vphysics
	// UNDONE: Solve this generally, systematically so other
	// NPCs can change size
	if ( lastHull != GetHullType() )
	{
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_PoisonZombie::HeadTarget( const Vector &posSrc )
{
	int iCrabAttachment = LookupAttachment( "headcrab1" );
	Assert( iCrabAttachment > 0 );

	Vector vecPosition;

	GetAttachment( iCrabAttachment, vecPosition );

	return vecPosition;
}

//-----------------------------------------------------------------------------
// Purpose: Turns off our breath so we can play another vocal sound.
//			TODO: pass in duration
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::BreatheOffShort( void )
{
	if ( m_bNearEnemy )
	{
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pFastBreathSound, SOUNDCTRL_CHANGE_VOLUME, envPoisonZombieBreatheVolumeOffShort, ARRAYSIZE(envPoisonZombieBreatheVolumeOffShort) );
	}
	else
	{
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pSlowBreathSound, SOUNDCTRL_CHANGE_VOLUME, envPoisonZombieBreatheVolumeOffShort, ARRAYSIZE(envPoisonZombieBreatheVolumeOffShort) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_ZOMBIE_POISON_SPIT )
	{
		Vector forward;
		QAngle qaPunch( 45, random->RandomInt(-5, 5), random->RandomInt(-5, 5) );
		AngleVectors( GetLocalAngles(), &forward );
		forward = forward * 200;
		ClawAttack( GetClawAttackRange(), sk_zombie_poison_dmg_spit.GetFloat(), qaPunch, forward, ZOMBIE_BLOOD_BITE );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::PrescheduleThink( void )
{
	bool bNearEnemy = false;
	if ( GetEnemy() != NULL )
	{
		float flDist = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();
		if ( flDist < ZOMBIE_ENEMY_BREATHE_DIST )
		{
			bNearEnemy = true;
		}
	}

	if ( bNearEnemy )
	{
		if ( !m_bNearEnemy )
		{
			// Our enemy is nearby. Breathe faster.
			float duration = random->RandomFloat( 1.0f, 2.0f );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pFastBreathSound, BREATH_VOL_MAX, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pFastBreathSound, random->RandomInt( 100, 120 ), random->RandomFloat( 1.0f, 2.0f ) );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pSlowBreathSound, 0.0f, duration );

			m_bNearEnemy = true;
		}
	}
	else if ( m_bNearEnemy )
	{
		// Our enemy is far away. Slow our breathing down.
		float duration = random->RandomFloat( 2.0f, 4.0f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pFastBreathSound, BREATH_VOL_MAX, duration );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pSlowBreathSound, 0.0f, duration );
//		ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 80, 100 ), duration );

		m_bNearEnemy = false;
	}

	BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_CHASE_ENEMY ) )
	{
		SetCustomInterruptCondition( COND_LIGHT_DAMAGE );
		SetCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
	/*else if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || IsCurSchedule( SCHED_RANGE_ATTACK2 ) )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_PoisonZombie::SelectSchedule( void )
{
	int nSchedule = BaseClass::SelectSchedule();

	if ( nSchedule == SCHED_SMALL_FLINCH )
	{
		 m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 1, 3 );
	}

	return nSchedule;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_PoisonZombie::TranslateSchedule( int scheduleType )
{
	/*if ( scheduleType == SCHED_RANGE_ATTACK2 )
	{
		return SCHED_ZOMBIE_POISON_RANGE_ATTACK2;
	}

	if ( scheduleType == SCHED_RANGE_ATTACK1 )
	{
		return SCHED_ZOMBIE_POISON_RANGE_ATTACK1;
	}*/

	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable( GetEnemy() ) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	// We'd simply like to shamble towards our enemy
	if ( scheduleType == SCHED_MOVE_TO_WEAPON_RANGE )
		return SCHED_CHASE_ENEMY;

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_PoisonZombie::ShouldPlayIdleSound( void )
{
	return CAI_BaseNPC::ShouldPlayIdleSound();
}


//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::AttackHitSound( void )
{
	EmitSound( "Zombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::AttackMissSound( void )
{
	EmitSound( "Zombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::AttackSound( void )
{
	EmitSound( "NPC_PoisonZombie.Attack" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::IdleSound( void )
{
	// HACK: base zombie code calls IdleSound even when not idle!
	if ( m_NPCState != NPC_STATE_COMBAT )
	{
		BreatheOffShort();
		EmitSound( "NPC_PoisonZombie.Idle" );
		MakeAISpookySound( 360.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random pain sound.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::PainSound( const CTakeDamageInfo &info )
{
	// Don't make pain sounds too often.
	if ( m_flNextPainSoundTime <= gpGlobals->curtime )
	{	
		BreatheOffShort();
		EmitSound( "NPC_PoisonZombie.Pain" );
		m_flNextPainSoundTime = gpGlobals->curtime + random->RandomFloat( 4.0, 7.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random alert sound.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::AlertSound( void )
{
	BreatheOffShort();

	EmitSound( "NPC_PoisonZombie.Alert" );
}


//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound( "NPC_PoisonZombie.FootstepRight" );
	}
	else
	{
		EmitSound( "NPC_PoisonZombie.FootstepLeft" );
	}

	if( ShouldPlayFootstepMoan() )
	{
		m_flNextMoanSound = gpGlobals->curtime;
		//MoanSound( envPoisonZombieMoanVolumeFast, ARRAYSIZE( envPoisonZombieMoanVolumeFast ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: If we don't have any headcrabs to throw, we must close to attack our enemy.
//-----------------------------------------------------------------------------
bool CNPC_PoisonZombie::MustCloseToAttack(void)
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Open a window and let a little bit of the looping moan sound
//			come through.
//-----------------------------------------------------------------------------
void CNPC_PoisonZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( !m_pMoanSound )
	{
		// Don't set this up until the code calls for it.
		const char *pszSound = GetMoanSound( m_iMoanSound );
		m_flMoanPitch = random->RandomInt( 98, 110 );

		CPASAttenuationFilter filter( this, 1.5 );
		//m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
		m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, 1.5 );

		ENVELOPE_CONTROLLER.Play( m_pMoanSound, 0.5, m_flMoanPitch );
	}

	envPoisonZombieMoanVolumeFast[ 1 ].durationMin = 0.1;
	envPoisonZombieMoanVolumeFast[ 1 ].durationMax = 0.4;

	if ( random->RandomInt( 1, 2 ) == 1 )
	{
		IdleSound();
	}

	float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

	float flPitchShift = random->RandomInt( -4, 4 );
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, m_flMoanPitch + flPitchShift, 0.3 );

	m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}

int ACT_ZOMBIE_POISON_THREAT;


AI_BEGIN_CUSTOM_NPC( npc_poisonzombie, CNPC_PoisonZombie )

	DECLARE_ACTIVITY( ACT_ZOMBIE_POISON_THREAT )

	//Adrian: events go here
	//DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_WARN_SOUND )
	//DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_PICKUP_CRAB )
	//DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_SOUND )
	//DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_CRAB )
	DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_SPIT )

    /*
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_POISON_RANGE_ATTACK2,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_FACE_IDEAL							0"
		"		TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_ZOMBIE_POISON_THREAT"
		"		TASK_FACE_IDEAL							0"
		"		TASK_RANGE_ATTACK2						0"

		"	Interrupts"
		"		COND_NO_PRIMARY_AMMO"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_POISON_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_NO_PRIMARY_AMMO"
	)
    */
AI_END_CUSTOM_NPC()


