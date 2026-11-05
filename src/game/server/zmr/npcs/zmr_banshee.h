#pragma once



#include "zmr_zombiebase.h"


class CZMBansheeMotor : public CZMBaseZombieMotor
{
public:
    typedef CZMBansheeMotor ThisClass;
    typedef CZMBaseZombieMotor BaseClass;

    CZMBansheeMotor( CZMBaseZombie* pOuter );

    virtual float GetHullHeight() const override;

    //virtual float GetFrictionForward() const override { return IsAttemptingToMove() ? 0.0f : 0.2f; }

    virtual void OnLandedGround( CBaseEntity* pGround ) override;
    virtual void NavJump( const Vector& vecGoal, float flOverrideHeight = 0.0f ) override;


    bool IsInNavJump() const { return m_bIsInNavJump; }

protected:
    virtual bool ShouldAdjustVelocity() const override;

private:
    bool m_bIsInNavJump;
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


    CZMBansheeMotor* GetBansheeMotor() const;
    virtual CZMBansheeMotor* CreateMotor() override { return new CZMBansheeMotor( this ); }


    virtual void Precache() override;
    virtual void Spawn() override;

    virtual void HandleAnimEvent( animevent_t* pEvent ) override;


    virtual NPCR::CSchedule<CZMBaseZombie>* OverrideCombatSchedule() const override;


    virtual void OnNavJump() override;


    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const override;
    virtual bool IsAttacking() const override;


    void StartCeilingAmbush( CZMPlayer* pCommander );
    bool LeapAttack( const QAngle& angPunch, const Vector& vecPunchVel, float flDamage );


    virtual bool    HasConditionsForRangeAttack( CBaseEntity* pEnemy ) const override;
    virtual NPCR::CSchedule<CZMBaseZombie>* GetRangeAttackSchedule() const override;


    virtual NPCR::CPathCostGroundOnly* GetPathCost() const override;
    virtual NPCR::CFollowNavPath* GetFollowPath() const override;


    float GetNextLeapAttack() const { return m_flNextLeapAttack; }
    void SetNextLeapAttack( float time ) { m_flNextLeapAttack = time; }


    virtual void OnAnimActivityFinished( Activity completedActivity ) override;


    virtual float GetClawAttackRange() const override { return 50.0f; }

    static float GetBansheeHullHeight() { return 52.0f; }
    static float GetMaxNavJumpDist() { return 512.0f; }
    static float GetMaxNavJumpHeight() { return 768.0f; }
    static float GetMinLeapAttackSpeed() { return 100.0f; }
    static float GetMaxLeapSpeed() { return 900.0f; }

    // Sounds
    virtual bool ShouldPlayIdleSound() const override;
    virtual float IdleSound() override;
    virtual void AlertSound() override;
    virtual void DeathSound() override;
    virtual void ClawImpactSound( bool bHit = true ) override;
    void LeapAttackSound();

private:
    float m_flNextLeapAttack;
    int m_nMeleeAttacks;


    NPCR::CSchedule<CZMBaseZombie>* m_pLeapSched;
    NPCR::CSchedule<CZMBaseZombie>* m_pCeilAmbushSched;
};
