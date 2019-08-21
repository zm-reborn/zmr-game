#pragma once


#include "npcr_component.h"

class CSound;
class CBaseEntity;

namespace NPCR
{
    class VisionEntity
    {
    public:
        VisionEntity( CBaseEntity* pEnt )
        {
            m_pEnt = pEnt;
            m_flLastSeen = gpGlobals->curtime;
        }
        ~VisionEntity() {}


        CBaseEntity*    GetEntity() const { return m_pEnt.Get(); }

        float   LastSeen() const { return m_flLastSeen; }
        void    UpdateLastSeen()
        {
            float newtime = gpGlobals->curtime;
            if ( newtime > m_flLastSeen )
                m_flLastSeen = newtime;
        }
        void    SetLastSeen( float lastseen ) { m_flLastSeen = lastseen; }

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

        virtual VisionEntity* GetEntityOf( CBaseEntity* pEnt ) const;
        virtual bool CanSee( CBaseEntity* pEnt ) const;
        virtual bool CanSee( VisionEntity* pVision ) const;
        virtual bool CanSee( const Vector& vecPos ) const;
        virtual bool HasLOS( const Vector& vecPos ) const; // Doesn't check for cone

        virtual int GetSoundMask() const;
        virtual bool CanHearSound( CSound* pSound ) const;


        virtual unsigned int GetVisionMask() const;
        virtual bool CanSeeThrough( CBaseEntity* pEnt ) const;

        virtual float GetVisionDistance() const { return 4096.0f; }
        virtual float GetFieldOfView() const { return 180.0f; }

        virtual CBaseEntity* GetClosestEntity() const;

        virtual bool IsValidVisionEntity( CBaseEntity* pEnt ) const;

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
