#pragma once

//#include "hl2/npc_BaseZombie.h"
#include "ai_basenpc.h"
#include "ai_blended_movement.h"
#include "ai_behavior_actbusy.h"
#include "soundenvelope.h"
#include "ehandle.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr_npclagcomp.h"


/*
    NOTE: You have to remove files from the project:

    npc_zombie.cpp
    npc_PoisonZombie.cpp
*/


#define ZOMBIE_MELEE_REACH      55


extern int AE_ZOMBIE_ATTACK_RIGHT;
extern int AE_ZOMBIE_ATTACK_LEFT;
extern int AE_ZOMBIE_ATTACK_BOTH;
extern int AE_ZOMBIE_SWATITEM;
extern int AE_ZOMBIE_STARTSWAT;
extern int AE_ZOMBIE_STEP_LEFT;
extern int AE_ZOMBIE_STEP_RIGHT;
extern int AE_ZOMBIE_SCUFF_LEFT;
extern int AE_ZOMBIE_SCUFF_RIGHT;
extern int AE_ZOMBIE_ATTACK_SCREAM;
extern int AE_ZOMBIE_GET_UP;
extern int AE_ZOMBIE_POUND;


// Pass these to claw attack so we know where to draw the blood.
#define ZOMBIE_BLOOD_LEFT_HAND      0
#define ZOMBIE_BLOOD_RIGHT_HAND     1
#define ZOMBIE_BLOOD_BOTH_HANDS     2
#define ZOMBIE_BLOOD_BITE           3


enum
{
    SCHED_ZOMBIE_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
    SCHED_ZOMBIE_MOVE_SWATITEM,
    SCHED_ZOMBIE_SWATITEM,
    SCHED_ZOMBIE_ATTACKITEM,
    //SCHED_ZOMBIE_RELEASECRAB,
    SCHED_ZOMBIE_MOVE_TO_AMBUSH,
    SCHED_ZOMBIE_WAIT_AMBUSH,
    SCHED_ZOMBIE_WANDER_MEDIUM,	// medium range wandering behavior.
    SCHED_ZOMBIE_WANDER_FAIL,
    SCHED_ZOMBIE_WANDER_STANDOFF,
    SCHED_ZOMBIE_MELEE_ATTACK1,
    SCHED_ZOMBIE_POST_MELEE_WAIT,

    SCHED_ZM_GO, // HACK
    SCHED_ZM_FORCED_GO,
    SCHED_ZM_DEFEND_GO_DEFPOS,
    SCHED_ZM_DEFEND_WAIT,
    SCHED_ZM_MOVE_AWAY,
    SCHED_ZM_AMBUSH_MODE,

    LAST_BASE_ZOMBIE_SCHEDULE,
};

enum 
{
    TASK_ZOMBIE_DELAY_SWAT = LAST_SHARED_TASK,
    TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ,
    TASK_ZOMBIE_SWAT_ITEM,
    TASK_ZOMBIE_DIE,
    //TASK_ZOMBIE_RELEASE_HEADCRAB,
    TASK_ZOMBIE_WAIT_POST_MELEE,

    TASK_ZM_DEFEND_PATH_TO_DEFPOS,
    TASK_ZM_SET_TOLERANCE_DISTANCE,

    LAST_BASE_ZOMBIE_TASK,
};

enum Zombie_Conds
{
    COND_ZOMBIE_CAN_SWAT_ATTACK = LAST_SHARED_CONDITION,
    //COND_ZOMBIE_RELEASECRAB,
    //COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION,

    COND_ZM_SEE_ENEMY,
    COND_ZM_DEFEND_ENEMY_CLOSE,
    COND_ZM_DEFEND_ENEMY_TOOFAR,
    //COND_ZM_FAILED_GOAL,
    COND_ZM_FAIL_AMBUSH,

    LAST_BASE_ZOMBIE_CONDITION,
};


class CZMPlayer;
class CZMEntAmbushTrigger;

typedef CAI_BlendingHost<CAI_BehaviorHost<CAI_BaseNPC>> CAI_BaseZombieBase;

class CZMBaseZombie : public CAI_BaseZombieBase, public CZMNPCLagCompensation
{
public:
    DECLARE_CLASS( CZMBaseZombie, CAI_BaseZombieBase )
    DECLARE_SERVERCLASS()
    DECLARE_DATADESC()
    DEFINE_CUSTOM_AI;

    CZMBaseZombie();
    ~CZMBaseZombie();
    

    virtual void Spawn() OVERRIDE;
    virtual void Precache() OVERRIDE;

    virtual int OnTakeDamage_Alive( const CTakeDamageInfo& ) OVERRIDE;
    virtual void HandleAnimEvent( animevent_t* ) OVERRIDE;
    
    virtual void    SetModel( const char* ) OVERRIDE;
    virtual void    SetZombieModel( void ) = 0;
    virtual Hull_t  GetZombieHull() { return HULL_HUMAN; };

    virtual bool ShouldGib( const CTakeDamageInfo& ) OVERRIDE;
    virtual bool CorpseGib( const CTakeDamageInfo &info ) OVERRIDE;

    
    virtual float MaxYawSpeed( void ) OVERRIDE;
    virtual bool OverrideMoveFacing( const AILocalMoveGoal_t&, float ) OVERRIDE;

    
    virtual void StartTask( const Task_t* ) OVERRIDE;
    virtual void RunTask( const Task_t* ) OVERRIDE;
    virtual void GatherConditions( void ) OVERRIDE;
    virtual int TranslateSchedule( int ) OVERRIDE;
    virtual void OnScheduleChange( void ) OVERRIDE;
    virtual int SelectSchedule( void ) OVERRIDE;
    virtual int SelectFailSchedule( int, int, AI_TaskFailureCode_t ) OVERRIDE;
    virtual void PrescheduleThink( void ) OVERRIDE;
    virtual bool OnInsufficientStopDist( AILocalMoveGoal_t*, float, AIMoveResult_t* ) OVERRIDE;
    virtual bool OnObstructionPreSteer( AILocalMoveGoal_t* pMoveGoal, float distClear, AIMoveResult_t* pResult ) OVERRIDE;


    int GetSwatActivity( void );
    // By default don't swat objects.
    virtual bool CanSwatPhysicsObjects() { return false; };
    bool FindNearestPhysicsObject( int iMaxMass );
    
    virtual float GetReactionDelay( CBaseEntity* pEnemy ) OVERRIDE { return 0.0; };
    virtual bool IsValidEnemy( CBaseEntity* ) OVERRIDE;
    // Always classify as a zombie. The AI relationships depend on it.
    virtual Class_T Classify( void ) OVERRIDE { return CLASS_ZOMBIE; };
    Disposition_t IRelationType( CBaseEntity* pTarget ) OVERRIDE { return CAI_BaseNPC::IRelationType( pTarget ); };

    virtual int RangeAttack1Conditions ( float flDot, float flDist ) OVERRIDE { return 0; };
    virtual int MeleeAttack1Conditions( float, float ) OVERRIDE;
    virtual float GetClawAttackRange() const { return ZOMBIE_MELEE_REACH; };
    virtual bool MustCloseToAttack() { return true; };
    virtual CBaseEntity* ClawAttack( float flDist, int iDamage, const QAngle& qaViewPunch, const Vector& vecVelocityPunch, int BloodOrigin );
    void GetAttackHull( Vector& mins, Vector& maxs );
    virtual const Vector GetAttackPos() const;

    virtual float GetHitgroupDamageMultiplier( int, const CTakeDamageInfo& ) OVERRIDE;
    virtual void TraceAttack( const CTakeDamageInfo&, const Vector&, trace_t*, CDmgAccumulator* ) OVERRIDE;



    // Let derived classes set this.
    bool ShouldPlayIdleSound( void ) { return false; };

    virtual void PainSound( const CTakeDamageInfo &info ) = 0;
    virtual void AlertSound( void ) = 0;
    virtual void IdleSound( void ) = 0;
    virtual void AttackSound( void ) = 0;
    virtual void AttackHitSound( void ) = 0;
    virtual void AttackMissSound( void ) = 0;
    virtual void FootstepSound( bool fRightFoot ) = 0;
    virtual void FootscuffSound( bool fRightFoot ) = 0;

    void PoundSound();
    void MakeAISpookySound( float = 0.0f ) {};


    void KillMe( void )
    {
        m_iHealth = 5;
        OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth * 2, DMG_GENERIC ) );
    }

    float DistToPhysicsEnt( void );


    virtual void BuildScheduleTestBits( void ) OVERRIDE;
    virtual void OnStateChange( NPC_STATE oldState, NPC_STATE newState ) OVERRIDE;
    virtual Activity NPC_TranslateActivity( Activity baseAct ) OVERRIDE;


    virtual	bool AllowedToIgnite( void ) OVERRIDE { return true; };
    bool ShouldIgnite( const CTakeDamageInfo& info );
    virtual void Ignite( float, bool = true, float = 0.0f, bool = false ) OVERRIDE;



    // Our stuff...
    int GetPopCost() { return CZMBaseZombie::GetPopCost( GetZombieClass() ); };
    int GetCost() { return CZMBaseZombie::GetCost( GetZombieClass() ); };
    ZombieClass_t GetZombieClass() { return m_iZombieClass; };

    
    // Player commanded means the zombie will ignore new enemies for a short period of time.
    virtual void Command( const Vector&, bool bPlayerCommanded = true, float tolerance = 0.0f );
    virtual bool Swat( CBaseEntity*, bool );
    void SwatObject( IPhysicsObject*, Vector& );
    virtual bool TargetEnemy( CBaseEntity* );


    bool CanSpawn( const Vector& );


    // Implemented in zmr_zombiebase_shared
    static bool IsValidClass( ZombieClass_t );
    static ZombieClass_t NameToClass( const char* );
    static const char* ClassToName( ZombieClass_t );
    static int GetPopCost( ZombieClass_t );
    static int GetCost( ZombieClass_t );
    static bool HasEnoughPopToSpawn( ZombieClass_t );
    int GetSelectorIndex();
    CZMPlayer* GetSelector();
    void SetSelector( CZMPlayer* );
    void SetSelector( int );


    inline bool IsCloseToCommandPos() { return IsCloseToPos( m_vecLastCommandPos ); };
    bool IsCloseToPos( const Vector& pos );
    inline bool HasBeenCommanded() { return m_bCommanded; };
    inline const Vector& GetLastCommandedPos() { return m_vecLastCommandPos; };
    inline ZombieMode_t GetZombieMode() { return m_iMode; };
    void SetZombieMode( ZombieMode_t mode );

    inline void SetAddGoalTolerance( float tolerance ) { m_flAddGoalTolerance = tolerance; };
    bool ShouldTryScheduleAgain( int failedSched, int failedTask, AI_TaskFailureCode_t );


    CZMEntAmbushTrigger* GetAmbushTrigger() { return m_hAmbushEnt.Get(); };
    void SetAmbush( CZMEntAmbushTrigger* trigger );
    void RemoveFromAmbush( bool bRemoveSched, bool bRemoveFromAmbushEnt = true );

public: // From base zombie...
    static int              ACT_ZOM_SWATLEFTMID;
    static int              ACT_ZOM_SWATRIGHTMID;
    static int              ACT_ZOM_SWATLEFTLOW;
    static int              ACT_ZOM_SWATRIGHTLOW;
    static int              ACT_ZOM_RELEASECRAB;
    static int              ACT_ZOM_FALL;


    CAI_ActBusyBehavior     m_ActBusyBehavior;

private:
    EHANDLE                 m_hPhysicsEnt;
    float                   m_flNextSwatScan;
    float                   m_flNextSwat;

    float                   m_flBurnDamage;
    float                   m_flBurnDamageResetTime;

    float                   m_flNextFlinch;

protected:
    //CSoundPatch*            m_pMoanSound;
    //float                   m_flNextIdleSound;
    //float                   m_flMoanPitch;
    //int                     m_iMoanSound;


protected: // Our stuff...
    inline int GetReturnSchedule() { return m_iReturnSchedule; };
    inline void SetReturnSchedule( int retSched ) { m_iReturnSchedule = retSched; };

private:
    void UpdateRetry( float enddelay = 15.0f );

    ZombieClass_t           m_iZombieClass;
    ZombieMode_t            m_iMode;
    

    float                   m_flLastCommand;
    Vector                  m_vecLastCommandPos;
    bool                    m_bCommanded;

    bool                    m_bSwatBreakable;
    float                   m_flAddGoalTolerance;

    // After completing a schedule, AI will return to this schedule.
    int                     m_iReturnSchedule;

    // If schedule fails, we can keep retrying it.
    int                     m_iScheduleRetry;
    float                   m_flRetryEndTime;
    Vector                  m_vecLastRetryPos;

    CHandle<CZMEntAmbushTrigger>    m_hAmbushEnt;
    
protected:
    inline void SetZombieClass( ZombieClass_t zclass ) { m_iZombieClass = zclass; };


    CNetworkVar( int, m_iSelectorIndex );
    CNetworkVar( float, m_flHealthRatio ); // For humans we can use health/maxhealth
};
