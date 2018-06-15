#pragma once



#include "zmr_zombiebase.h"


class CZMBansheeMotor : public NPCR::CNonPlayerMotor
{
public:
    CZMBansheeMotor( CZMBaseZombie* pOuter );

    virtual float GetHullHeight() const OVERRIDE;

    //virtual float GetFrictionForward() const OVERRIDE { return IsAttemptingToMove() ? 0.0f : 0.2f; }
};

class CZMBanshee : public CZMBaseZombie
{
public:
    typedef CZMBanshee ThisClass;
    typedef CZMBaseZombie BaseClass;
    //DECLARE_CLASS( CZMBanshee, CZMBaseZombie );
    DECLARE_SERVERCLASS();

    CZMBanshee();
    ~CZMBanshee();


    virtual CZMBansheeMotor* CreateMotor() OVERRIDE { return new CZMBansheeMotor( this ); }


    virtual void Precache() OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;


    virtual NPCR::CSchedule<CZMBaseZombie>* OverrideCombatSchedule() const OVERRIDE;


    virtual void OnNavJump() OVERRIDE;


    virtual bool IsAttacking() const OVERRIDE;


    void StartCeilingAmbush();
    bool LeapAttack( const QAngle& angPunch, const Vector& vecPunchVel, float flDamage );


    virtual bool    HasConditionsForRangeAttack( CBaseEntity* pEnemy ) const OVERRIDE;
    virtual NPCR::CSchedule<CZMBaseZombie>* GetRangeAttackSchedule() const OVERRIDE;


    virtual NPCR::CPathCostGroundOnly* GetPathCost() const OVERRIDE;
    virtual NPCR::CFollowNavPath* GetFollowPath() const OVERRIDE;


    float GetNextLeapAttack() const { return m_flNextLeapAttack; }
    void SetNextLeapAttack( float time ) { m_flNextLeapAttack = time; }


    virtual void OnAnimActivityFinished( Activity completedActivity ) OVERRIDE;


    virtual float GetClawAttackRange() const OVERRIDE { return 50.0f; }

    static float GetBansheeHullHeight() { return 52.0f; }
    static float GetMaxNavJumpDist() { return 512.0f; }
    static float GetMaxNavJumpHeight() { return 768.0f; }
    static float GetMinLeapAttackSpeed() { return 100.0f; }
    static float GetMaxLeapSpeed() { return 900.0f; }

    // Sounds
    virtual void AlertSound() OVERRIDE;
    virtual void AttackSound() OVERRIDE;
    virtual void DeathSound() OVERRIDE;
    virtual void FootstepSound( bool bRightFoot = false ) OVERRIDE;
    virtual void FootscuffSound( bool bRightFoot = false ) OVERRIDE;
    virtual void ClawImpactSound( bool bHit = true ) OVERRIDE;
    void LeapAttackSound();

private:
    float m_flNextLeapAttack;


    NPCR::CSchedule<CZMBaseZombie>* m_pLeapSched;
    NPCR::CSchedule<CZMBaseZombie>* m_pCeilAmbushSched;


public:
    static Activity ACT_FASTZOMBIE_FRENZY;
    static Activity ACT_FASTZOMBIE_BIG_SLASH;

    static Activity ACT_FASTZOMBIE_LAND_RIGHT;
    static Activity ACT_FASTZOMBIE_LAND_LEFT;
    static Activity ACT_FASTZOMBIE_LEAP_STRIKE;


    static int AE_FASTZOMBIE_GALLOP_LEFT;
    static int AE_FASTZOMBIE_GALLOP_RIGHT;
    static int AE_FASTZOMBIE_LEAP;
};
