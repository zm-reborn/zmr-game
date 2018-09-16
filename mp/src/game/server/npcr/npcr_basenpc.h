#pragma once

#include "npcr_component.h"


class CBaseCombatCharacter;

namespace NPCR
{
    class CScheduleInterface;
    class CBaseMotor;
    class CBaseSenses;
    class CFollowNavPath;

    abstract_class CNPCInterface
    {
    public:
        CNPCInterface( CBaseCombatCharacter* pChar )
        {
            m_pCurrentPath = nullptr;
            m_pCombatCharacter = pChar;
        }


        CBaseCombatCharacter* GetCharacter() const { return m_pCombatCharacter; }


        const Vector& GetPosition() { return GetCharacter()->GetLocalOrigin(); }
        void SetPosition( const Vector& vecPos ) { GetCharacter()->SetLocalOrigin( vecPos ); }
        const QAngle& GetAngles() { return GetCharacter()->GetLocalAngles(); }
        void SetAngles( const QAngle& ang ) { GetCharacter()->SetLocalAngles( ang ); }
        const QAngle& GetEyeAngles() { return GetCharacter()->EyeAngles(); }
        virtual void SetEyeAngles( const QAngle& ang ) { SetAngles( ang ); }
        const Vector& GetVel() { return GetCharacter()->GetLocalVelocity(); }
        void SetVel( const Vector& vecVel ) { GetCharacter()->SetLocalVelocity( vecVel ); }


        virtual bool IsEnemy( CBaseEntity* pEnt ) const { return false; }
        virtual bool IsTargetedEnemy( CBaseEntity* pEnt ) const { return false; }


        NPCR::CFollowNavPath*   GetCurrentPath() const { return m_pCurrentPath; }
        void                    SetCurrentPath( NPCR::CFollowNavPath* path ) { m_pCurrentPath = path; }
    private:
        NPCR::CFollowNavPath* m_pCurrentPath;

        CBaseCombatCharacter* m_pCombatCharacter;
    };

    abstract_class CBaseComponents
    {
    public:
        CBaseComponents()
        {
            m_pSchedInterface = nullptr;
            m_pMotor = nullptr;
            m_pSenses = nullptr;
        }

        virtual ~CBaseComponents();

        
        CScheduleInterface* GetScheduleInterface() const;
        CBaseMotor*         GetMotor() const;
        CBaseSenses*        GetSenses() const;
        
    protected:
        virtual bool                CreateComponents();
        virtual CScheduleInterface* CreateScheduleInterface() { return nullptr; }
        virtual CBaseMotor*         CreateMotor() { return nullptr; }
        virtual CBaseSenses*        CreateSenses() { return nullptr; }
        virtual void                OnCreatedComponents() = 0;

    private:
        CScheduleInterface* m_pSchedInterface;
        CBaseMotor* m_pMotor;
        CBaseSenses* m_pSenses;
    };

    class CBaseNPC : public CEventDispatcher, public CNPCInterface, public CBaseComponents
    {
    public:
        DECLARE_CLASS_NOBASE( CBaseNPC )

        CBaseNPC( CBaseCombatCharacter* pChar );
        ~CBaseNPC();

        float GetUpdateInterval() const { return m_flUpdateTime; }
        float GetLastUpdateTime() const { return m_flLastUpdate; }

        virtual bool RemoveNPC() { return false; }

    protected:
        void PostConstructor();
        virtual void OnCreatedComponents() OVERRIDE;

        virtual bool ShouldUpdate() const;


        virtual void PreUpdate() {}
        virtual void Update() OVERRIDE;
        virtual void PostUpdate() {}
    private:
        void UpdateInterval();
        void FinishUpdate();

        int m_iLastUpdateTick;
        float m_flLastUpdate;
        float m_flUpdateTime;
        int m_iUpdateSlot;

        bool m_bFlaggedForUpdate;


        friend class NPCManager;
    };
}
