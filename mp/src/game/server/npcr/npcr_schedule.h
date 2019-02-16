#pragma once


#include "cbase.h"

#include "npcr_component.h"


namespace NPCR
{
    class CScheduleInterface;

    enum ScheduleState_t
    {
        SCHED_INSTART = 0, // We haven't started yet.
        SCHED_INPROGRESS, // We're running!
        SCHED_INTERCEPTED, // We've been stopped by a child schedule.
        SCHED_DONE, // We're done, waiting for release.
    };

    enum EventState_t
    {
        SCHED_EVENT_OK = 0, // Everything's good, keep going.
        SCHED_EVENT_INTERCEPT, // We want to be intercepted next update.
        SCHED_EVENT_DONE // We want to be done next update.
    };

    class CScheduleInt : public CEventListener
    {
    private:
        virtual void StartInitialSchedule() = 0;

        template<typename NPCRChar>
        friend class CSchedule;
        friend class CScheduleInterface;
    };

    template<typename NPCRChar>
    class CSchedule : public CScheduleInt
    {
    public:
        CSchedule()
        {
            m_State = SCHED_INSTART;

            m_EventState = SCHED_EVENT_OK;
            m_pNextSchedule = nullptr;
            m_pszEventReason = "";

            m_pChild = nullptr;
            m_pParent = nullptr;
            m_pFriendSched = nullptr;
            m_pParentInterface = nullptr;
            m_pOuter = nullptr;
        }
        ~CSchedule()
        {
            delete m_pFriendSched;
            m_pFriendSched = nullptr;

            //delete m_pChild;
            //m_pChild = nullptr;
        }
        

        virtual const char* GetName() const { return ""; }

        virtual NPCRChar* GetOuter() const OVERRIDE { return m_pOuter; }


        bool HasStarted() const { return m_State != SCHED_INSTART; }
        bool IsDone() const { return m_State == SCHED_DONE; }
        bool IsIntercepted() const { return m_State == SCHED_INTERCEPTED; }
        bool IsRunning() const { return m_State == SCHED_INPROGRESS; }

        virtual void OnStart() {}
        virtual void OnUpdate() {}
        virtual void OnEnd() {}
        virtual void OnContinue() {} // When the child schedule has finished.
        

        CSchedule<NPCRChar>* GetActiveSchedule() const
        {
            if ( IsIntercepted() )
            {
                Assert( m_pChild );
                return m_pChild->GetActiveSchedule();
            }

            return const_cast<CSchedule<NPCRChar>*>( this );
        }

        
        ScheduleState_t GetState() const { return m_State; }
        EventState_t GetEventState() const { return m_EventState; }

        static bool IsDebugging()
        {
            extern ConVar npcr_debug_schedules;
            return npcr_debug_schedules.GetInt() > 0;
        }

    protected:
        virtual CSchedule<NPCRChar>* CreateFriendSchedule() { return nullptr; }

        void TryIntercept( CSchedule<NPCRChar>* sched, const char* szReason = "" )
        {
            if ( m_EventState != SCHED_EVENT_OK )
            {
                AssertMsg( 0, szReason );
                return;
            }

            Assert( sched != nullptr );

            if ( IsDebugging() )
                Msg( "[NPCR] Trying to intercept '%s' with '%s' because '%s'.\n", GetName(), sched->GetName(), szReason );


            m_EventState = SCHED_EVENT_INTERCEPT;
            m_pNextSchedule = sched;
            m_pszEventReason = szReason;
        }

        ScheduleState_t Intercept( CSchedule<NPCRChar>* sched, const char* szReason = "" )
        {
            // Bad practice if we attempt this more than once... or a bug.
            if ( IsIntercepted() )
            {
                AssertMsg( 0, szReason );
                return SCHED_INTERCEPTED;
            }

            Assert( sched != nullptr );

            if ( IsDebugging() )
                Msg( "[NPCR] Intercepted '%s' with '%s' because '%s'.\n", GetName(), sched->GetName(), szReason );


            sched->m_pParent = this;
            sched->m_pChild = nullptr;
            ScheduleState_t retState = sched->RunStart( GetOuter(), GetParentInterface() );
            
            if ( retState == SCHED_DONE )
            {
                return retState;
            }
            

            GetParentInterface()->RemoveComponent( this );


            m_State = SCHED_INTERCEPTED;
            m_pChild = sched;
            return retState;
        }

        void TryEnd( const char* szReason = "" )
        {
            if ( m_EventState != SCHED_EVENT_OK )
            {
                AssertMsg( 0, szReason );
                return;
            }

            if ( IsDebugging() )
                Msg( "[NPCR] Trying to end '%s' because '%s'.\n", GetName(), szReason );


            m_EventState = SCHED_EVENT_DONE;
            m_pszEventReason = szReason;
        }

        void End( const char* szReason = "" )
        {
            // Bad practice if we attempt this more than once... or a bug.
            if ( IsDone() )
            {
                AssertMsg( 0, szReason );
                return;
            }

            if ( IsDebugging() )
                Msg( "[NPCR] Ended '%s' because '%s'.\n", GetName(), szReason );


            m_State = SCHED_DONE;


            if ( m_pChild )
            {
                m_pChild->m_pParent = nullptr;
                m_pChild->End( szReason );
            }

            m_pChild = nullptr;


            if ( m_pFriendSched )
            {
                m_pFriendSched->End( szReason );
            }
        }

    private:
        CSchedule<NPCRChar>* GetFriendSchedule() const { return m_pFriendSched; }

        CScheduleInterface* GetParentInterface() const { return m_pParentInterface; }


        void SetOuter( NPCRChar* pOuter ) { m_pOuter = pOuter; }


        void RunEnd()
        {
            GetParentInterface()->RemoveComponent( this );

            OnEnd();


            if ( m_pParent )
            {
                m_pParent->RunContinue();
            }
        }

        void RunContinue()
        {
            // Bad practice if we attempt this more than once... or a bug.
            if ( m_State == SCHED_INPROGRESS )
            {
                Assert( 0 );
                return;
            }


            m_pChild = nullptr;
            m_State = SCHED_INPROGRESS;


            GetParentInterface()->AddComponent( this );


            if ( IsDebugging() )
                Msg( "[NPCR] Continuing '%s'\n", GetName() );

            OnContinue();
        }

        ScheduleState_t RunStart( NPCRChar* pOuter, CScheduleInterface* pParentInterface )
        {
            Assert( pOuter );
            Assert( pParentInterface );
            SetOuter( pOuter );
            m_pParentInterface = pParentInterface;


            m_pFriendSched = CreateFriendSchedule();
            if ( m_pFriendSched )
            {
                m_pFriendSched->RunStart( pOuter, pParentInterface );
            }
            

            m_EventState = SCHED_EVENT_OK;
            m_State = SCHED_INPROGRESS;
            OnStart();


            CSchedule<NPCRChar>* newSched = GetActiveSchedule();
            if ( newSched->IsRunning() )
            {
                GetParentInterface()->RemoveComponent( this );
                GetParentInterface()->AddComponent( newSched );
            }


            return newSched->GetState();
        }


        virtual void StartInitialSchedule() OVERRIDE
        {
            auto* pChar = dynamic_cast<NPCRChar*>( GetNPC()->GetOuter() );
            auto* pSchedInt = dynamic_cast<CScheduleInterface*>( CComponent::GetOwner() );
            RunStart( pChar, pSchedInt );
        }

        virtual void Update() OVERRIDE
        {
            // Starts from the initial schedule.
            // Run through all the other schedules.
            CSchedule<NPCRChar>* pFirst = this;

            while ( pFirst )
            {
                CSchedule<NPCRChar>* pActive;
                
                pActive = pFirst->GetActiveSchedule();

                if ( !pActive->HasStarted() )
                {
                    pActive->RunStart( GetOuter(), GetParentInterface() );

                    // Might get updated.
                    pActive = pActive->GetActiveSchedule();
                }


                // Handle any event requests.
                while ( pActive->GetEventState() != SCHED_EVENT_OK )
                {
                    if ( pActive->GetEventState() == SCHED_EVENT_INTERCEPT )
                    {
                        pActive->Intercept( m_pNextSchedule, m_pszEventReason );
                    }
                    else if ( pActive->GetEventState() == SCHED_EVENT_DONE )
                    {
                        pActive->End( m_pszEventReason );
                    }


                    pActive->m_EventState = SCHED_EVENT_OK;

                    pActive = pActive->GetActiveSchedule();
                }

                // Loop through all the done schedules.
                while ( pActive->IsDone() )
                {
                    pActive->RunEnd();
                    CSchedule<NPCRChar>* next = pFirst->GetActiveSchedule();

                    if ( next == pActive )
                        break;

                    pActive = next;
                }


                if ( pActive->IsRunning() )
                {
                    pActive->OnUpdate();
                }

                pFirst = pFirst->GetFriendSchedule();
            }
        }

    
        NPCRChar* m_pOuter;
        ScheduleState_t m_State;

        EventState_t m_EventState;
        const char* m_pszEventReason;
        CSchedule<NPCRChar>* m_pNextSchedule;

        CSchedule<NPCRChar>* m_pChild;
        CSchedule<NPCRChar>* m_pParent;
        CSchedule<NPCRChar>* m_pFriendSched;
        CScheduleInterface* m_pParentInterface;
    };


    class CScheduleInterface : public CEventDispatcher
    {
    public:
        CScheduleInterface( CBaseNPC* pNPC, CScheduleInt* pInitialSched );
        ~CScheduleInterface();

        virtual const char* GetComponentName() const OVERRIDE { return "ScheduleInterface"; }


    protected:
        virtual void Update() OVERRIDE;

    private:
        CScheduleInt* m_pInitialSched;
    };
}
