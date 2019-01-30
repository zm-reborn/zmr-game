#pragma once


#include "npcr_schedule.h"
#include "npcr_nonplayer.h"
#include "npcr_motor_nonplayer.h"
#include "npcr_senses.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"
#include "zmr/npcs/zmr_npclagcomp.h"
#include "zmr_cmd_queue.h"

//#include "zmr_blockerfinder.h"

#include "zmr_shareddefs.h"
#include "zmr/zmr_player.h"


class CZMZombieAnimState;
class CZMBlockerScanner;
class CZMEntAmbushTrigger;


class CZMBaseZombieMotor : public NPCR::CNonPlayerMotor
{
public:
    CZMBaseZombieMotor( CZMBaseZombie* pOuter );

    virtual float GetStepHeight() const OVERRIDE;
};


class CZMBaseZombie : public CNPCRNonPlayer, public CZMNPCLagCompensation
{
public:
    typedef CZMBaseZombie ThisClass;
    typedef CNPCRNonPlayer BaseClass;
    //DECLARE_CLASS( CZMBaseZombie, CNPCRNonPlayer );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();


    CZMBaseZombie();
    ~CZMBaseZombie();


    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    void ZombieThink();
    virtual void PreUpdate() OVERRIDE;
    virtual void PostUpdate() OVERRIDE;


    virtual bool IsBaseZombie() const OVERRIDE { return true; }
    virtual Class_T Classify() OVERRIDE { return CLASS_ZOMBIE; }


    // Components
    virtual bool CreateComponents() OVERRIDE;

    virtual CZMBlockerScanner*          CreateBlockerScanner();
    virtual CZMZombieAnimState*         CreateAnimState();

    virtual NPCR::CScheduleInterface*   CreateScheduleInterface() OVERRIDE;
    virtual NPCR::CBaseSenses*          CreateSenses() OVERRIDE;
    virtual NPCR::CNonPlayerMotor*      CreateMotor() OVERRIDE;

    CZMBlockerScanner*                  GetBlockerFinder() const { return m_pBlockerScanner; }

    virtual bool ShouldUpdate() const OVERRIDE;

    virtual NPCR::QueryResult_t ShouldTouch( CBaseEntity* pEnt ) const OVERRIDE;


    virtual NPCR::CSchedule<CZMBaseZombie>* OverrideCombatSchedule() const { return nullptr; }

    virtual NPCR::CFollowNavPath* GetFollowPath() const;


    // Damage/death related
    virtual void    Extinguish() OVERRIDE;
    virtual void    Ignite( float flFlameLifetime, bool bNPCOnly = false, float flSize = 0.0f, bool bCalledByLevelDesigner = false ) OVERRIDE;
    bool            ShouldIgnite( const CTakeDamageInfo& info );

    virtual void            Event_Killed( const CTakeDamageInfo& info ) OVERRIDE;
    virtual int             OnTakeDamage_Alive( const CTakeDamageInfo& inputInfo ) OVERRIDE;
    virtual void            TraceAttack( const CTakeDamageInfo& inputInfo, const Vector& vecDir, trace_t* pTrace, CDmgAccumulator* pAccumulator ) OVERRIDE;
    virtual bool            ScaleDamageByHitgroup( int iHitGroup, CTakeDamageInfo& info ) const;
    void                    MultiplyBuckshotDamage( CTakeDamageInfo& info ) const;


    virtual bool            Event_Gibbed( const CTakeDamageInfo& info ) OVERRIDE;
    virtual bool            CorpseGib( const CTakeDamageInfo& info ) OVERRIDE;
    virtual bool            ShouldGib( const CTakeDamageInfo& info ) OVERRIDE;



    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;


    // Sounds
    virtual bool ShouldPlayIdleSound() const;
    virtual float IdleSound() { return 0.0f; } // Return delay for the next idle sound.
    virtual void AlertSound() {}
    virtual void DeathSound() {}

    virtual void ClawImpactSound( bool bHit = true );




    // Implemented in zmr_zombiebase_shared
    static bool             IsValidClass( ZombieClass_t zclass );
    static ZombieClass_t    NameToClass( const char* name );
    static const char*      ClassToName( ZombieClass_t zclass );
    static int              GetPopCost( ZombieClass_t zclass );
    static int              GetCost( ZombieClass_t zclass );
    static float            GetSpawnDelay( ZombieClass_t zclass );
    static bool             HasEnoughPopToSpawn( ZombieClass_t zclass );
    int                     GetSelectorIndex() const;
    CZMPlayer*              GetSelector() const;
    void                    SetSelector( CZMPlayer* pPlayer );
    void                    SetSelector( int index );
    ZombieClass_t           GetZombieClass() const;
    int                     GetPopCost() const;
    int                     GetCost() const;
    bool                    DoAnimationEvent( int iEvent, int nData = 0 );
    virtual int             GetAnimationRandomSeed() OVERRIDE;
    virtual bool            CanBePenetrated() const;
protected:
    void                    SetZombieClass( ZombieClass_t zclass );

    int m_iAdditionalAnimRandomSeed;
public:

    static void GetAnimRandomSeed( int iEvent, int& nData );


    static float GetSwatMaxMass();
    virtual bool Swat( CZMPlayer* pZM, CBaseEntity* pSwat, bool bBreak = false );
    virtual bool SwatObject( CBaseEntity* pSwat );
    virtual void Command( CZMPlayer* pZM, const Vector& vecPos, float flTolerance = 0.0f );
    virtual Activity GetSwatActivity( CBaseEntity* pEnt, bool bBreak = true ) const { return ACT_MELEE_ATTACK1; }

    static bool     CanSwatObject( CBaseEntity* pEnt );
    virtual bool    CanBreakObject( CBaseEntity* pEnt, bool bSwat = false ) const;
    virtual bool    CanSwatPhysicsObjects() const { return false; }
    //float           GetNextSwatScan() { return m_flNextSwatScan; }
    //void            SetNextSwatScan( float time ) { m_flNextSwatScan = time; }
    CBaseEntity*    GetSwatObject() const { return m_hSwatObject.Get(); }
    void            SetSwatObject( CBaseEntity* pEnt ) { m_hSwatObject.Set( pEnt ); }



    CZMEntAmbushTrigger*    GetAmbushTrigger() { return m_hAmbushEnt.Get(); };
    void                    SetAmbush( CZMEntAmbushTrigger* trigger );
    void                    RemoveFromAmbush( bool bRemoveFromAmbushEnt = true );


    bool CanSpawn( const Vector& vecPos ) const;
    virtual bool    IsAttacking() const;
    virtual bool    CanMove() const;




    float           GetLastTimeCommanded() const { return m_flLastCommanded; }
    void            UpdateLastTimeCommanded() { m_flLastCommanded = gpGlobals->curtime; }

    virtual bool    IsEnemy( CBaseEntity* pEnt ) const OVERRIDE;


    virtual float GetMoveActivityMovementSpeed() OVERRIDE;


    virtual NPCR::CPathCostGroundOnly* GetPathCost() const;


    // Attacks
    virtual void            GetAttackHull( Vector& mins, Vector& maxs ) const;
    virtual const Vector    GetAttackPos() const;
    virtual float           GetAttackHeight() const;
    virtual float           GetAttackLowest() const;
    virtual float           GetClawAttackRange() const;
    virtual CBaseEntity*    ClawAttack( float flDist, float flDamage, const QAngle& angPunch, const Vector& vecPunchVel );

    virtual bool    HasConditionsForClawAttack( CBaseEntity* pEnemy ) const;

    virtual bool    CanRangeAttack() const { return false; }
    virtual bool    HasConditionsForRangeAttack( CBaseEntity* pEnemy ) const { return false; }
    virtual NPCR::CSchedule<CZMBaseZombie>* GetRangeAttackSchedule() const { return nullptr; }


    inline float    GetNextAttack() const { return m_flNextAttack; }
    inline void     SetNextAttack( float time ) { m_flNextAttack = time; }


    ZombieMode_t    GetZombieMode() const { return m_iZombieMode; }
    void            SetZombieMode( ZombieMode_t mode ) { m_iZombieMode = mode; }


    CZMCommandQueue*    GetCommandQueue() const { return const_cast<CZMCommandQueue*>( &m_CmdQueue ); }


    const char* GetZombieModelGroupName() const { return STRING( m_strModelGroup ); }
    void SetZombieModelGroupName( string_t name ) { m_strModelGroup = name; }
    //void SetZombieModelGroupName( const char* name ) { m_strModelGroup = AllocPooledString( name ); }
private:
    string_t m_strModelGroup;
public:


    float   GetNextMove() const { return m_flNextMove; }
    void    SetNextMove( float time ){ m_flNextMove = time; }


    // Hull trace that starts from our origin.
    bool MeleeAttackTrace(
        const Vector& vecMins, const Vector& vecMaxs,
        float flDist,
        float flDamage, int iDmgType = DMG_SLASH,
        CUtlVector<CBaseEntity*>* vHitEnts = nullptr,
        const Vector* vecDir = nullptr );


protected:
    CZMZombieAnimState* GetAnimState() const { return m_pAnimState; }


private:
    float m_flNextAttack;

    float m_flLastCommanded;


    CZMBlockerScanner* m_pBlockerScanner;
    CZMZombieAnimState* m_pAnimState;

    CHandle<CBaseEntity> m_hSwatObject;


    ZombieMode_t m_iZombieMode;
    ZombieClass_t m_iZombieClass;


    CNetworkVar( int, m_iSelectorIndex );
    CNetworkVar( float, m_flHealthRatio ); // For humans we can use health/maxhealth
    CNetworkVar( bool, m_bIsOnGround );
    CNetworkVar( int, m_iAnimationRandomSeed );



    CHandle<CZMEntAmbushTrigger> m_hAmbushEnt;



    CZMCommandQueue m_CmdQueue;



    float m_flNextMove;


    float m_flBurnDamage;
    float m_flBurnDamageTime;

    float m_flNextIdleSound;


public:
    static int AE_ZOMBIE_ATTACK_RIGHT;
    static int AE_ZOMBIE_ATTACK_LEFT;
    static int AE_ZOMBIE_ATTACK_BOTH;
    static int AE_ZOMBIE_SWATITEM;
    static int AE_ZOMBIE_GET_UP;
    static int AE_ZOMBIE_POUND;
    static int AE_ZOMBIE_ALERTSOUND;
};

inline CZMBaseZombie* ToZMBaseZombie( CBaseEntity* pEnt )
{
    if ( !pEnt || !pEnt->IsBaseZombie() )
        return nullptr;

    return static_cast<CZMBaseZombie*>( pEnt );
}
