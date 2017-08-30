//=============================================================================
// Copyright (c) Zombie Master Development Team. All rights reserved.
// The use and distribution terms for this software are covered by the MIT
// License (http://opensource.org/licenses/mit-license.php) which
// can be found in the file LICENSE.TXT at the root of this distribution. By
// using this software in any fashion, you are agreeing to be bound by the
// terms of this license. You must not remove this notice, or any other, from
// this software.
//
// Note that due to the number of files included in the SDK, it is not feasible
// to include this notice in all of them. All original files or files 
// containing large modifications should contain this notice. If in doubt,
// assume the above notice applies, and refer to the included LICENSE.TXT text.

//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "game.h"
//#include "npc_headcrab.h"
#include "npcevent.h"
#include "entitylist.h"
#include "ai_task.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"


#include "zmr/zmr_gamerules.h"
#include "zmr_zombiebase.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BREATH_VOL_MAX  0.6
//TGB: upped range a bit now that their health is lower
#define DRAGZOMBIE_SPITRANGE 120.0f

#define DRAGZOMBIE_ENEMY_BREATHE_DIST		300	// How close we must be to our enemy before we start breathing hard.
int AE_DRAGGY_SICK;

//static ConVar zm_popcost_drifter("zm_popcost_drifter", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Population points taken up by drifters");

//-----------------------------------------------------------------------------
// The model we use for our legs when we get blowed up.
//-----------------------------------------------------------------------------
static const char *s_szLegsModel = "models/zombie/classic_legs.mdl";


//-----------------------------------------------------------------------------
// The classname of the headcrab that jumps off of this kind of zombie.
//-----------------------------------------------------------------------------
//static const char *s_szHeadcrabClassname = "npc_headcrab_poison";
//static const char *s_szHeadcrabModel = "models/headcrabblack.mdl";

static const char *pMoanSounds[] =
{
    "NPC_DragZombie.Moan1",
};

//-----------------------------------------------------------------------------
// Skill settings.
//-----------------------------------------------------------------------------
extern ConVar zm_sk_dragzombie_health;
extern ConVar zm_sk_dragzombie_dmg;
//ConVar sk_dragzombie_dmg_spit( "sk_dragzombie_poison_dmg_spit","0");

class CNPC_DragZombie : public CAI_BlendingHost<CZMBaseZombie>
{
public:
    DECLARE_CLASS( CNPC_DragZombie, CAI_BlendingHost<CZMBaseZombie> );
    //DECLARE_SERVERCLASS()
    //DECLARE_DATADESC()
    DEFINE_CUSTOM_AI // Register the spit effect.


    CNPC_DragZombie()
    {
        SetZombieClass( ZMCLASS_DRIFTER );
        CZMRules::IncPopCount( GetZombieClass() );
    }



    virtual float MaxYawSpeed( void );

    virtual float GetClawAttackRange() const { return 70; }

    virtual void PrescheduleThink( void );
    virtual void BuildScheduleTestBits( void );
    virtual int SelectSchedule( void );
    virtual int SelectFailSchedule( int nFailedSchedule, int nFailedTask, AI_TaskFailureCode_t eTaskFailCode );

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
    virtual int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

    void PainSound( const CTakeDamageInfo &info );
    void AlertSound( void );
    void IdleSound( void );
    void AttackSound( void );
    void AttackHitSound( void );
    void AttackMissSound( void );
    void FootstepSound( bool fRightFoot );
    void FootscuffSound( bool fRightFoot ) {};

    virtual void StopLoopingSounds( void );
    int   m_nSquidSpitSprite;


protected:

    virtual const char *GetMoanSound( int nSoundIndex );
    virtual const char *GetLegsModel( void );
    virtual const char *GetTorsoModel( void );

    int MeleeAttack1Conditions ( float flDot, float flDist );
private:

    void BreatheOffShort( void );

};

LINK_ENTITY_TO_CLASS( npc_dragzombie, CNPC_DragZombie );


//BEGIN_DATADESC( CNPC_DragZombie )
//END_DATADESC()

//IMPLEMENT_SERVERCLASS_ST( CNPC_DragZombie, DT_NPC_DragZombie )
//END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::Precache( void )
{
    PrecacheModel("models/humans/zm_draggy.mdl");
    m_nSquidSpitSprite = PrecacheModel("sprites/laserdot.vmt"); // client side spittle.
    PrecacheScriptSound( "NPC_DragZombie.Die" );
    PrecacheScriptSound( "NPC_DragZombie.Idle" );
    PrecacheScriptSound( "NPC_DragZombie.Pain" );
    PrecacheScriptSound( "NPC_DragZombie.Alert" );
    PrecacheScriptSound( "NPC_DragZombie.FootstepRight" );
    PrecacheScriptSound( "NPC_DragZombie.FootstepLeft" );
    PrecacheScriptSound( "NPC_DragZombie.Attack" );
    PrecacheScriptSound( "NPC_DragZombie.MeleeAttack" );

    PrecacheScriptSound( "NPC_DragZombie.FastBreath" );
    PrecacheScriptSound( "NPC_DragZombie.Moan1" );

    PrecacheScriptSound( "Zombie.AttackHit" );
    PrecacheScriptSound( "Zombie.AttackMiss" );

    BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::Spawn( void )
{
    Precache();

    m_fIsTorso = m_fIsHeadless = false;

    SetBloodColor( BLOOD_COLOR_RED );
    m_iHealth = zm_sk_dragzombie_health.GetFloat();
    m_flFieldOfView = 0.1f;

    CapabilitiesClear();
    CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );

    BaseClass::Spawn();

    CPASAttenuationFilter filter( this, ATTN_IDLE );
/*	m_pFastBreathSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_ITEM, "NPC_DragZombie.FastBreath", ATTN_IDLE );
    ENVELOPE_CONTROLLER.Play( m_pFastBreathSound, 0.0f, 100 );

    CPASAttenuationFilter filter2( this );
    m_pSlowBreathSound = ENVELOPE_CONTROLLER.SoundCreate( filter2, entindex(), CHAN_ITEM, "NPC_DragZombie.Moan1", ATTN_NORM );
    ENVELOPE_CONTROLLER.Play( m_pSlowBreathSound, BREATH_VOL_MAX, 100 );
*/
/*	int nCrabs = m_nCrabCount;
    if ( !nCrabs )
    {
        nCrabs = MAX_CRABS;
    }
    m_nCrabCount = 0;
*/
    //
    // Generate a random set of crabs based on the crab count
    // specified by the level designer.
    //
/*	int nBits[] = 
    {
        // One bit
        0x01,
        0x02,
        0x04,

        // Two bits
        0x03,
        0x05,
        0x06,
    };

    int nBitMask = 7;
    if (nCrabs == 1)
    {
        nBitMask = nBits[random->RandomInt( 0, 2 )];
    }
    else if (nCrabs == 2)
    {
        nBitMask = nBits[random->RandomInt( 3, 5 )];
    }

    for ( int i = 0; i < MAX_CRABS; i++ )
    {
        EnableCrab( i, ( nBitMask & ( 1 << i ) ) != 0 );
    }*/
}


//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CNPC_DragZombie::GetMoanSound( int nSound )
{
    return pMoanSounds[nSound % ARRAYSIZE( pMoanSounds )];
}


//-----------------------------------------------------------------------------
// Purpose: Returns the model to use for our legs ragdoll when we are blown in twain.
//-----------------------------------------------------------------------------
const char *CNPC_DragZombie::GetLegsModel( void )
{
    return s_szLegsModel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char *CNPC_DragZombie::GetTorsoModel( void )
{
    return "models/zombie/classic_torso.mdl";
}



//-----------------------------------------------------------------------------
// Purpose: Returns the classname (ie "npc_headcrab") to spawn when our headcrab bails.
//-----------------------------------------------------------------------------
/*const char *CNPC_DragZombie::GetHeadcrabClassname( void )
{
    return s_szHeadcrabClassname;
}

const char *CNPC_DragZombie::GetHeadcrabModel( void )
{
    return s_szHeadcrabModel;
}
*/

//-----------------------------------------------------------------------------
// Purpose: Turns the given crab on or off.
//-----------------------------------------------------------------------------
/*void CNPC_DragZombie::EnableCrab( int nCrab, bool bEnable )
{
    ASSERT( ( nCrab >= 0 ) && ( nCrab < MAX_CRABS ) );

    if ( ( nCrab >= 0 ) && ( nCrab < MAX_CRABS ) )
    {
        if (m_bCrabs[nCrab] != bEnable)
        {
            m_nCrabCount += bEnable ? 1 : -1;
        }

        m_bCrabs[nCrab] = bEnable;
        SetBodygroup( ZOMBIE_BODYGROUP_NEST_BASE + nCrab, bEnable );
    }
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::StopLoopingSounds( void )
{
/*	ENVELOPE_CONTROLLER.SoundDestroy( m_pFastBreathSound );
    m_pFastBreathSound = NULL;

    ENVELOPE_CONTROLLER.SoundDestroy( m_pSlowBreathSound );
    m_pSlowBreathSound = NULL;
*/
    BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : info - 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::Event_Killed( const CTakeDamageInfo &info )
{
    if ( !( info.GetDamageType() & ( DMG_BLAST | DMG_ALWAYSGIB) ) ) 
    {
        EmitSound( "NPC_DragZombie.Die" );
    }

/*	if ( !m_fIsTorso )
    {
        EvacuateNest(info.GetDamageType() == DMG_BLAST, info.GetDamage(), info.GetAttacker() );
    }
*/
    BaseClass::Event_Killed( info );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputInfo - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_DragZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
/*	//
    // Calculate what percentage of the creature's max health
    // this amount of damage represents (clips at 1.0).
    //
    float flDamagePercent = min( 1, inputInfo.GetDamage() / m_iMaxHealth );

    //
    // Throw one crab for every 20% damage we take.
    //
    if ( flDamagePercent >= 0.2 )
    {
        m_flNextCrabThrowTime = gpGlobals->curtime;
    }
*/
    return BaseClass::OnTakeDamage_Alive( inputInfo );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_DragZombie::MaxYawSpeed( void )
{
    return BaseClass::MaxYawSpeed();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// NOTE: This function is still heavy with common code (found at the bottom).
//		 we should consider moving some into the base class! (sjb)
//-----------------------------------------------------------------------------
void CNPC_DragZombie::SetZombieModel( void )
{
    Hull_t lastHull = GetHullType();
        SetModel( "models/humans/zm_draggy.mdl" );
        SetHullType(HULL_HUMAN);

//	SetBodygroup( ZOMBIE_BODYGROUP_HEADCRAB, !m_fIsHeadless );

    SetHullSizeNormal( true );
    SetDefaultEyeOffset();
    SetActivity( ACT_IDLE );

    //set random skin, will have no effect if the model doesn't specify more than one (which it doesn't by def.)
    m_nSkin = random->RandomInt( 0, 3 );

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
// Purpose: Checks conditions for letting a headcrab leap off our back at our enemy.
//-----------------------------------------------------------------------------
/*int CNPC_DragZombie::RangeAttack1Conditions( float flDot, float flDist )
{
    if ( !m_nCrabCount )
    {
        //DevMsg("Range1: No crabs\n");
        return 0;
    }

    if ( m_flNextCrabThrowTime > gpGlobals->curtime )
    {
        //DevMsg("Range1: Too soon\n");
        return 0;
    }

    if ( flDist < ZOMBIE_HC_LEAP_RANGE_MIN )
    {
        //DevMsg("Range1: Too close to attack\n");
        return COND_TOO_CLOSE_TO_ATTACK;
    }
    
    if ( flDist > ZOMBIE_HC_LEAP_RANGE_MAX )
    {
        //DevMsg("Range1: Too far to attack\n");
        return COND_TOO_FAR_TO_ATTACK;
    }

    if ( flDot < ZOMBIE_HC_LEAP_CONE )
    {
        //DevMsg("Range1: Not facing\n");
        return COND_NOT_FACING_ATTACK;
    }

    m_nThrowCrab = RandomThrowCrab();

    //DevMsg("*** Range1: Can range attack\n");
    return COND_CAN_RANGE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: Checks conditions for throwing a headcrab leap at our enemy.
//-----------------------------------------------------------------------------
int CNPC_DragZombie::RangeAttack2Conditions( float flDot, float flDist )
{
    if ( !m_nCrabCount )
    {
        //DevMsg("Range2: No crabs\n");
        return 0;
    }

    if ( m_flNextCrabThrowTime > gpGlobals->curtime )
    {
        //DevMsg("Range2: Too soon\n");
        return 0;
    }

    if ( flDist < ZOMBIE_THROW_RANGE_MIN )
    {
        //DevMsg("Range2: Too close to attack\n");
        return COND_TOO_CLOSE_TO_ATTACK;
    }
    
    if ( flDist > ZOMBIE_THROW_RANGE_MAX )
    {
        //DevMsg("Range2: Too far to attack\n");
        return COND_TOO_FAR_TO_ATTACK;
    }

    if ( flDot < ZOMBIE_THROW_CONE )
    {
        //DevMsg("Range2: Not facing\n");
        return COND_NOT_FACING_ATTACK;
    }

    m_nThrowCrab = RandomThrowCrab();

    //DevMsg("*** Range2: Can range attack\n");
    return COND_CAN_RANGE_ATTACK2;
}
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*Vector CNPC_DragZombie::HeadTarget( const Vector &posSrc )
{
    int iCrabAttachment = LookupAttachment( "headcrab1" );
    Assert( iCrabAttachment > 0 );

    Vector vecPosition;
    QAngle angles;

    GetAttachment( iCrabAttachment, vecPosition, angles );

    return vecPosition;
}
*/
//-----------------------------------------------------------------------------
// Purpose: Turns off our breath so we can play another vocal sound.
//			TODO: pass in duration
//-----------------------------------------------------------------------------
void CNPC_DragZombie::BreatheOffShort( void )
{
/*	if ( m_bNearEnemy )
    {
        ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pFastBreathSound, SOUNDCTRL_CHANGE_VOLUME, envDragZombieBreatheVolumeOffShort, ARRAYSIZE(envDragZombieBreatheVolumeOffShort) );
    }
    else
    {
        ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pSlowBreathSound, SOUNDCTRL_CHANGE_VOLUME, envDragZombieBreatheVolumeOffShort, ARRAYSIZE(envDragZombieBreatheVolumeOffShort) );
    }*/
}


//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::HandleAnimEvent( animevent_t *pEvent )
{
    if ( pEvent->event == AE_DRAGGY_SICK )
    {
        
        //LAWYER:  We need to play a "Vomit" thing here
        if ( GetEnemy() )
        {
            Vector vSpitPos;
            QAngle vSpitAngle;
            Vector vSpitDir;
            Vector vecForceDir;

            GetAttachment( "Mouth", vSpitPos, vSpitAngle);
            
            Vector			vTarget = GetEnemy()->GetAbsOrigin();

            /* TGB: doesn't seem to do anything relevant
            Vector			vToss;
            CBaseEntity*	pBlocker;
            float flGravity  = 5;
            ThrowLimit(vSpitPos, vTarget, flGravity, 3, Vector(0,0,0), Vector(0,0,0), GetEnemy(), &vToss, &pBlocker);
            */

            AngleVectors(vSpitAngle, &vSpitDir);
            //CPVSFilter filter( vSpitPos );

            UTIL_BloodSpray( vSpitPos, vSpitDir, BLOOD_COLOR_RED, RandomInt( 4, 16 ), FX_BLOODSPRAY_ALL);

            CBaseEntity *pHurt = CheckTraceHullAttack( DRAGZOMBIE_SPITRANGE + 10, -Vector(16,16,32), Vector(16,16,32), zm_sk_dragzombie_dmg.GetInt(), DMG_ACID, 5.0f );

            if ( pHurt )
            {
                vecForceDir = ( pHurt->WorldSpaceCenter() - WorldSpaceCenter() );
                
                CBasePlayer *pPlayer = ToBasePlayer( pHurt );

                if ( pPlayer != NULL )
                {
                    //Kick the player angles
                    pPlayer->ViewPunch( QAngle((random->RandomFloat(-50.0f, 50.0f)),(random->RandomFloat(-50.0f, 50.0f)),(random->RandomFloat(-50.0f, 50.0f))) );

                    // ZMRTODO: Make this less annoying?
                    pPlayer->SetAbsVelocity(Vector(0,0,0));

                    Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();
                    VectorNormalize(dir);

                    QAngle angles;
                    VectorAngles( dir, angles );
                    Vector forward, right;
                    AngleVectors( angles, &forward, &right, NULL );

                    //Push the target back
                    //pHurt->ApplyAbsVelocityImpulse( - right * shove[1] - forward * shove[0] );
                    pPlayer->SetAbsAngles(QAngle(pPlayer->GetAbsAngles().x + random->RandomInt(-10,10), pPlayer->GetAbsAngles().y+ random->RandomInt(-10,10), pPlayer->GetAbsAngles().z));
                    

                    //TGB: draw some blood on him
                    const float flNoise = 2.0;
                    //Vector vecTraceDir = pHurt->EyePosition();
                    //seems to be too high in some cases, so lower a notch
                    //vecTraceDir.z -= 4.0f;
                    Vector vecTraceDir = vSpitPos + (forward * (DRAGZOMBIE_SPITRANGE + 30));
                    vecTraceDir.x += random->RandomFloat( -flNoise, flNoise );
                    vecTraceDir.y += random->RandomFloat( -flNoise, flNoise );
                    vecTraceDir.z += random->RandomFloat( -flNoise, flNoise );

                    trace_t traceHit;
                    UTIL_TraceLine( vSpitPos, vecTraceDir, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &traceHit );
                    DoImpactEffect( traceHit, DMG_BULLET );
                }

                // Play a random attack hit sound
                EmitSound( "NPC_DragZombie.MeleeAttack" );
            }
        }
        //ClawAttack( GetClawAttackRange(), sk_dragzombie_poison_dmg_spit.GetFloat(), qaPunch, forward, ZOMBIE_BLOOD_BITE );
        return;
        //Warning("VOMIT!");
    }

    BaseClass::HandleAnimEvent( pEvent );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the index of a randomly chosen crab to throw.
//-----------------------------------------------------------------------------
/*int CNPC_DragZombie::RandomThrowCrab( void )
{
    // FIXME: this could take a long time, theoretically
    int nCrab = -1;
    do
    {
        int nTest = random->RandomInt( 0, 2 );
        if ( m_bCrabs[nTest] )
        {
            nCrab = nTest;
        }
    } while ( nCrab == -1 );
    
    return nCrab;
}
*/

//-----------------------------------------------------------------------------
// Purpose: The nest is dead! Evacuate the nest!
// Input  : bExplosion - We were evicted by an explosion so we should go a-flying.
//			flDamage - The damage that was done to cause the evacuation.
//-----------------------------------------------------------------------------
/*void CNPC_DragZombie::EvacuateNest( bool bExplosion, float flDamage, CBaseEntity *pAttacker )
{
    // HACK: if we were in mid-throw, drop the throwing crab also.
    if ( GetBodygroup( ZOMBIE_BODYGROUP_THROW ) )
    {
        SetBodygroup( ZOMBIE_BODYGROUP_THROW, 0 );
        m_nCrabCount++;
    }

    for( int i = 0; i < MAX_CRABS ; i++ )
    {
        if( m_bCrabs[i] )
        {
            Vector vecPosition;
            QAngle vecAngles;

            char szAttachment[64];

            switch( i )
            {
            case 0:
                strcpy( szAttachment, "headcrab2" );
                break;
            case 1:
                strcpy( szAttachment, "headcrab3" );
                break;
            case 2:
                strcpy( szAttachment, "headcrab4" );
                break;
            }

            GetAttachment( szAttachment, vecPosition, vecAngles );

            // Now slam the angles because the attachment point will have pitch and roll, which we can't use.
            vecAngles = QAngle( 0, random->RandomFloat( 0, 360 ), 0 );

            CBlackHeadcrab *pCrab = (CBlackHeadcrab *)CreateNoSpawn( GetHeadcrabClassname(), vecPosition, vecAngles, this );
            pCrab->Spawn();

            if( !HeadcrabFits(pCrab) )
            {
                UTIL_Remove(pCrab);
                continue;
            }

            float flVelocityScale = 2.0f;
            if ( bExplosion && ( flDamage > 10 ) )
            {
                flVelocityScale = 0.1 * flDamage;
            }

            if (IsOnFire())
            {
                pCrab->Ignite( 100.0 );
            }

            pCrab->Eject( vecAngles, flVelocityScale, pAttacker );
            EnableCrab( i, false );
        }
    }
}

*/
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_DragZombie::PrescheduleThink( void )
{
/*	if ( HasCondition( COND_NEW_ENEMY ) )
    {
        m_flNextCrabThrowTime = gpGlobals->curtime + random->RandomInt( ZOMBIE_THROW_FIRST_MIN_DELAY, ZOMBIE_THROW_FIRST_MAX_DELAY );
    }
*/
//	bool bNearEnemy = false;
/*
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
//		float duration = random->RandomFloat( 2.0f, 4.0f );
/*		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pFastBreathSound, BREATH_VOL_MAX, duration );
        ENVELOPE_CONTROLLER.SoundChangeVolume( m_pSlowBreathSound, 0.0f, duration );
//		ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 80, 100 ), duration );

        m_bNearEnemy = false;
    }
*/
    BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_DragZombie::BuildScheduleTestBits( void )
{
    BaseClass::BuildScheduleTestBits();

    if ( IsCurSchedule( SCHED_CHASE_ENEMY ) )
    {
        SetCustomInterruptCondition( COND_LIGHT_DAMAGE );
        SetCustomInterruptCondition( COND_HEAVY_DAMAGE );
    }
/*	else if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || IsCurSchedule( SCHED_RANGE_ATTACK2 ) )
    {
        ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
        ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
    }*/
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_DragZombie::SelectFailSchedule( int nFailedSchedule, int nFailedTask, AI_TaskFailureCode_t eTaskFailCode )
{
    int nSchedule = BaseClass::SelectFailSchedule( nFailedSchedule, nFailedTask, eTaskFailCode );

/*	if ( nSchedule == SCHED_CHASE_ENEMY_FAILED && m_nCrabCount > 0 )
    {
        return SCHED_ESTABLISH_LINE_OF_FIRE;
    }
*/
    return nSchedule;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_DragZombie::SelectSchedule( void )
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
//-----------------------------------------------------------------------------
bool CNPC_DragZombie::ShouldPlayIdleSound( void )
{
    return CAI_BaseNPC::ShouldPlayIdleSound();
}


//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CNPC_DragZombie::AttackHitSound( void )
{
    EmitSound( "Zombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CNPC_DragZombie::AttackMissSound( void )
{
    EmitSound( "Zombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CNPC_DragZombie::AttackSound( void )
{
    EmitSound( "NPC_DragZombie.Attack" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CNPC_DragZombie::IdleSound( void )
{
    // HACK: base zombie code calls IdleSound even when not idle!
    if ( m_NPCState != NPC_STATE_COMBAT )
    {
        BreatheOffShort();
        EmitSound( "NPC_DragZombie.Idle" );
    }
}

//-----------------------------------------------------------------------------
// Purpose: Play a random pain sound.
//-----------------------------------------------------------------------------
void CNPC_DragZombie::PainSound( const CTakeDamageInfo &info )
{
    //TGB: we don't have a pain sound and the hulk's doesn't really fit
    /*
    // Don't make pain sounds too often.
    if ( m_flNextPainSoundTime <= gpGlobals->curtime )
    {	
        BreatheOffShort();
        EmitSound( "NPC_DragZombie.Pain" );
        m_flNextPainSoundTime = gpGlobals->curtime + random->RandomFloat( 4.0, 7.0 );
    }*/
}

//-----------------------------------------------------------------------------
// Purpose: Play a random alert sound.
//-----------------------------------------------------------------------------
void CNPC_DragZombie::AlertSound( void )
{
    BreatheOffShort();

    EmitSound( "NPC_DragZombie.Alert" );
}


//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CNPC_DragZombie::FootstepSound( bool fRightFoot )
{
/*	if( fRightFoot )
    {
        EmitSound( "NPC_DragZombie.FootstepRight" );
    }
    else
    {
        EmitSound( "NPC_DragZombie.FootstepLeft" );
    }
*/
    if( ShouldPlayFootstepMoan() )
    {
        m_flNextMoanSound = gpGlobals->curtime;
//		MoanSound( envDragZombieMoanVolumeFast, ARRAYSIZE( envDragZombieMoanVolumeFast ) );
    }
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------

int CNPC_DragZombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
    if (flDist > DRAGZOMBIE_SPITRANGE )
    {
/*		// Special check vs players inside vehicles
        if ( GetEnemy() && GetEnemy()->IsPlayer() )
        {
            CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );
            if ( pPlayer->IsInAVehicle() )
                return MeleeAttack1ConditionsVsPlayerInVehicle( pPlayer, flDot );
        }
*/
        return COND_TOO_FAR_TO_ATTACK;
    }

    if (flDot < 0.7)
    {
        return COND_NOT_FACING_ATTACK;
    }

    // Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
    Vector vecMins = GetHullMins();
    Vector vecMaxs = GetHullMaxs();
    vecMins.z = vecMins.x;
    vecMaxs.z = vecMaxs.x;

    Vector forward;
    GetVectors( &forward, NULL, NULL );

    trace_t	tr;
    CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
    AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * DRAGZOMBIE_SPITRANGE, vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

    if( tr.fraction == 1.0 || !tr.m_pEnt )
    {
        // This attack would miss completely. Trick the zombie into moving around some more.
        return COND_TOO_FAR_TO_ATTACK;
    }

    if( tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES))
    {
        // -Let the zombie swipe at his enemy if he's going to hit them.
        // -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
        //  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
        // -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
        return COND_CAN_MELEE_ATTACK1;
    }

/*
    if( tr.m_pEnt->IsBSPModel() )
    {
        // The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
        // the enemy is, treat this as an obstruction.
        Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
        Vector vecTrace = tr.endpos - tr.startpos;

        if( vecTrace.Length2DSqr() < vecToEnemy.Length2DSqr() )
        {
            return COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION;
        }
    }
*/
    // Move around some more
    return COND_TOO_FAR_TO_ATTACK;
}

int ACT_DRAGZOMBIE_POISON_THREAT;


AI_BEGIN_CUSTOM_NPC( npc_dragzombie, CNPC_DragZombie )

    DECLARE_ACTIVITY( ACT_DRAGZOMBIE_POISON_THREAT )

    //Adrian: events go here
//	DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_WARN_SOUND )
//	DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_PICKUP_CRAB )
//	DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_SOUND )
//	DECLARE_ANIMEVENT( AE_ZOMBIE_POISON_THROW_CRAB )
    DECLARE_ANIMEVENT( AE_DRAGGY_SICK )

/*	DEFINE_SCHEDULE
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
*/
AI_END_CUSTOM_NPC()

