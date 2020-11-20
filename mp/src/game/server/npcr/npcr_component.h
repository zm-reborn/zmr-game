#pragma once


//#include "npcr_events.h"


enum ZombieCommandType_t : unsigned char;

class CBaseCombatCharacter;

namespace NPCR
{
    class CBaseNPC;
    class CEventDispatcher;
    class CBaseNavPath;

    enum QueryResult_t
    {
        RES_NO = 0,
        RES_YES,
        RES_NONE
    };

    abstract_class EventComponent
    {
    public:
        EventComponent();


        // Events
        virtual void OnSpawn() {}
        virtual void OnAnimEvent( animevent_t* pEvent ) {}
        virtual void OnAnimActivityInterrupted( Activity newActivity ) {}
        virtual void OnAnimActivityFinished( Activity completedActivity ) {}
        virtual void OnSightGained( CBaseEntity* pEnt ) {}
        virtual void OnSightLost( CBaseEntity* pEnt ) {}
        virtual void OnAcquiredEnemy( CBaseEntity* pEnt ) {} // A new enemy acquired!
        virtual void OnLostEnemy( CBaseEntity* pEnt ) {} // We no longer know about this enemy.
        virtual void OnDamaged( const CTakeDamageInfo& info ) {}
        virtual void OnHeardSound( CSound* pSound ) {}
        virtual void OnChase( CBaseEntity* pEnt ) {} // When enemy is being chased.
        virtual void OnLandedGround( CBaseEntity* pGround ) {}
        virtual void OnLeftGround( CBaseEntity* pOldGround ) {}
        virtual void OnTouch( CBaseEntity* pEnt, trace_t* pTrace ) {}
        virtual void OnMoveSuccess( CBaseNavPath* pPath ) {}
        virtual void OnMoveFailed( CBaseNavPath* pPath ) {}
        virtual void OnNavJump() {}
        virtual void OnForcedMove( CNavArea* pArea ) {} // Used for debugging.
        virtual void OnStuck() {} // We've been stuck for a while.
        // Zombie events
        virtual void OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t com ) {}
        virtual void OnQueuedCommand( CBasePlayer* pCommander, ZombieCommandType_t com ) {}
        virtual void OnAttacked() {}


        // Queries
        virtual QueryResult_t IsBusy() const { return RES_NONE; }
        virtual QueryResult_t ShouldTouch( CBaseEntity* pEnt ) const { return RES_NONE; }
        // Zombie queries
        virtual QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const { return RES_NONE; }

    protected:
        bool IsSelfCall() const { return m_bSelfCall; }
        void SetSelfCall( bool state ) { m_bSelfCall = state; }

    private:
        bool m_bSelfCall;
    };

    abstract_class CComponent : public EventComponent
    {
    public:
        CComponent( CEventDispatcher* pOwner = nullptr, CBaseNPC* pNPC = nullptr );
        virtual ~CComponent();


        virtual const char* GetComponentName() const { return ""; }


        CBaseNPC* GetNPC() const;
        virtual CBaseCombatCharacter* GetOuter() const;


        CComponent* GetFriendComponent() const;


        virtual void Start() {}
        virtual void Update() {}

    private:
        CEventDispatcher*  GetOwner() const { return m_pOwner; }


        CEventDispatcher* m_pOwner;
        CBaseNPC* m_pNPC;

        friend class CEventListener;
        friend class CEventDispatcher;
        template<typename T>
        friend class CSchedule;
    };

    abstract_class CEventListener : public CComponent
    {
    public:
        CEventListener( CEventDispatcher* pOwner = nullptr, CBaseNPC* pNPC = nullptr );
    };

    abstract_class CEventDispatcher : public CComponent
    {
    public:
        // Send 
        CEventDispatcher( CEventDispatcher* pOwner = nullptr, CBaseNPC* pNPC = nullptr );


        template<typename ForEachFunc>
        void ForEachComponent( ForEachFunc func )
        {
            int len = m_Components.Count();
            for ( int i = 0; i < len; ++i )
            {
                func( m_Components[i] );
            }
        }

        template<typename ForEachFunc>
        QueryResult_t ForEachComponentQuery( ForEachFunc func ) const
        {
            QueryResult_t res;
            int len = m_Components.Count();
            for ( int i = 0; i < len; ++i )
            {
                res = func( m_Components[i] );
                if ( res != RES_NONE )
                    return res;
            }

            return RES_NONE;
        }

    private:
        friend class CComponent;
        friend class CScheduleInterface;
        template<typename T>
        friend class CSchedule;
        friend class CBaseNPC;


        void AddComponent( CComponent* c );
        void RemoveComponent( CComponent* c );
        int FindComponent( CComponent* c ) const;

        CComponent* NextComponent( CComponent* c ) const;
        CComponent* PreviousComponent( CComponent* c ) const;


        CUtlVector<CComponent*> m_Components;
    public:

#define COMP_DISPATCH_Q(func)                                   QueryResult_t func() const OVERRIDE { return ForEachComponentQuery( []( CComponent* c ) { return c->func(); } ); }
#define COMP_DISPATCH_Q_1ARG(func,argtype1,arg1)                QueryResult_t func( argtype1 arg1 ) const OVERRIDE { return ForEachComponentQuery( [ &arg1 ]( CComponent* c ) { return c->func( arg1 ); } ); }
#define COMP_DISPATCH(func)                                     void func() OVERRIDE { ForEachComponent( []( CComponent* c ) { c->func(); } ); }
#define COMP_DISPATCH_1ARG(func,argtype1,arg1)                  void func( argtype1 arg1 ) OVERRIDE { ForEachComponent( [ &arg1 ]( CComponent* c ) { c->func( arg1 ); } ); }
#define COMP_DISPATCH_2ARG(func,argtype1,arg1,argtype2,arg2)    void func( argtype1 arg1, argtype2 arg2 ) OVERRIDE { ForEachComponent( [ &arg1, &arg2 ]( CComponent* c ) { c->func( arg1, arg2 ); } ); }

        void Update() OVERRIDE { ForEachComponent( []( CComponent* c ){ c->Update(); } ); }
        

        // Events
        COMP_DISPATCH( OnSpawn )
        COMP_DISPATCH( OnNavJump )
        COMP_DISPATCH_1ARG( OnMoveSuccess, CBaseNavPath*, pPath )
        COMP_DISPATCH_1ARG( OnMoveFailed, CBaseNavPath*, pPath )
        COMP_DISPATCH_1ARG( OnAnimEvent, animevent_t*, pEvent )
        COMP_DISPATCH_1ARG( OnAnimActivityInterrupted, Activity, newActivity )
        COMP_DISPATCH_1ARG( OnAnimActivityFinished, Activity, completedActivity )
        COMP_DISPATCH_1ARG( OnSightGained, CBaseEntity*, pEnt )
        COMP_DISPATCH_1ARG( OnSightLost, CBaseEntity*, pEnt )
        COMP_DISPATCH_1ARG( OnAcquiredEnemy, CBaseEntity*, pEnt )
        COMP_DISPATCH_1ARG( OnLostEnemy, CBaseEntity*, pEnt )
        COMP_DISPATCH_1ARG( OnDamaged, const CTakeDamageInfo&, info )
        COMP_DISPATCH_1ARG( OnHeardSound, CSound*, pSound )
        COMP_DISPATCH_1ARG( OnChase, CBaseEntity*, pEnt )
        COMP_DISPATCH_1ARG( OnLandedGround, CBaseEntity*, pGround )
        COMP_DISPATCH_1ARG( OnLeftGround, CBaseEntity*, pOldGround )
        COMP_DISPATCH_2ARG( OnTouch, CBaseEntity*, pEnt, trace_t*, pTrace )
        COMP_DISPATCH_1ARG( OnForcedMove, CNavArea*, pArea )
        // Zombie events
        COMP_DISPATCH_2ARG( OnCommanded, CBasePlayer*, pCommander, ZombieCommandType_t, com )
        COMP_DISPATCH_2ARG( OnQueuedCommand, CBasePlayer*, pCommander, ZombieCommandType_t, com )
        COMP_DISPATCH( OnAttacked )
        COMP_DISPATCH( OnStuck )

        // Queries
        COMP_DISPATCH_Q( IsBusy )
        COMP_DISPATCH_Q_1ARG( ShouldTouch, CBaseEntity*, pEnt )
        // Zombie queries
        COMP_DISPATCH_Q_1ARG( ShouldChase, CBaseEntity*, pEnemy )
    };
}
