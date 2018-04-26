#pragma once


#include "npcr_component.h"

class CSound;
class CBaseEntity;

namespace NPCR
{
    class CBaseNonPlayer;


    class VisionEntity
    {
    public:
        VisionEntity( CBaseEntity* pEnt )
        {
            m_pEnt = pEnt;
            UpdateLastSeen();
        }
        ~VisionEntity() {}


        CBaseEntity*    GetEntity() { return m_pEnt.Get(); };

        float           LastSeen() { return m_flLastSeen; };
        void            UpdateLastSeen() { m_flLastSeen = gpGlobals->curtime; };

    private:
        CHandle<CBaseEntity> m_pEnt;
        float m_flLastSeen;
    };

    
    class CBaseSenses : public CEventListener
    {
    public:
        CBaseSenses( CBaseNPC* pNPC );
        ~CBaseSenses();


        virtual const char* GetComponentName() const OVERRIDE { return "BaseSenses"; }


        virtual void Update() OVERRIDE;

        virtual bool CanSee( CBaseEntity* pEnt ) const;
        virtual bool CanSee( VisionEntity* pVision ) const;
        virtual bool CanSee( const Vector& vecPos ) const;
        virtual bool HasLOS( const Vector& vecPos ) const; // Doesn't check for cone

        virtual bool CanHearSound( CSound* pSound ) const;

        virtual float GetVisionDistance() const { return 4096.0f; }
        virtual float GetFieldOfView() const { return 180.0f; }

        virtual CBaseEntity* GetClosestEntity() const;

    protected:
        virtual bool ShouldUpdateVision();
        virtual bool ShouldUpdateHearing();

        virtual void UpdateHearing();

        virtual void UpdateVision();
        virtual void FindNewEntities( CUtlVector<CBaseEntity*>& vListEnts );


        CUtlVector<VisionEntity*> m_vVisionEnts;
        float m_flFovCos;

        CountdownTimer m_NextVisionTimer;
        CountdownTimer m_NextHearingTimer;
    };
}
