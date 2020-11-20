#pragma once


#include "npcr_component.h"

class CSound;
class CBaseEntity;

namespace NPCR
{
    //
    // An entity we know about.
    // Holds information like last seen or known (heard / perhaps damaged).
    // Known entities will eventually expire once
    // no new information is received for a while.
    //
    class KnownEntity
    {
    public:
        KnownEntity( CBaseEntity* pEnt, bool bSees )
        {
            m_pEnt = pEnt;
            m_flAcquiredTime = gpGlobals->curtime;

            UpdateLastKnown();

            m_bCanSee = bSees;

            if ( bSees )
            {
                UpdateLastSeen();
            }
        }
        ~KnownEntity() {}


        CBaseEntity*    GetEntity() const { return m_pEnt.Get(); }

        float   AcquiredTime() const { return m_flAcquiredTime; }
        float   LastSeenTime() const { return m_flLastSeenTime; }
        float   LastKnownTime() const { return m_flLastKnownTime; }
        bool    CanSee() const { return m_bCanSee; }

        void UpdateLastSeen()
        {
            UpdateLastKnown();

            float newtime = gpGlobals->curtime;
            if ( newtime > m_flLastSeenTime )
                m_flLastSeenTime = newtime;

            m_bCanSee = true;
        }

        void SetCantSee()
        {
            m_bCanSee = false;
        }

        void UpdateLastKnown()
        {
            float newtime = gpGlobals->curtime;
            if ( newtime > m_flLastKnownTime )
                m_flLastKnownTime = newtime;
        }

    private:
        CHandle<CBaseEntity> m_pEnt;

        // Last time we saw the entity.
        float m_flLastSeenTime;

        // Last time we heard/saw the entity.
        float m_flLastKnownTime;

        // When we acquired this entity.
        float m_flAcquiredTime;

        bool m_bCanSee;
    };

    //
    // Interface to senses, like sight and hearing.
    // Holds a list of known entities.
    //
    class CBaseSenses : public CEventListener
    {
    public:
        CBaseSenses( CBaseNPC* pNPC );
        ~CBaseSenses();


        virtual const char* GetComponentName() const OVERRIDE { return "BaseSenses"; }


        virtual void Update() OVERRIDE;

        const KnownEntity* GetKnownOf( CBaseEntity* pEnt ) const;

        virtual bool CanSee( CBaseEntity* pEnt ) const;
        virtual bool CanSee( KnownEntity* pKnown ) const;
        virtual bool CanSee( const Vector& vecPos ) const;

        // Doesn't check field of view.
        virtual bool HasLOS( const Vector& vecPos ) const;

        virtual int GetSoundMask() const;
        virtual bool CanHearSound( CSound* pSound ) const;


        virtual unsigned int GetVisionMask() const;
        virtual bool CanSeeThrough( CBaseEntity* pEnt ) const;

        virtual float GetVisionDistance() const { return 4096.0f; }
        virtual float GetFieldOfView() const { return 180.0f; }

        virtual CBaseEntity* GetClosestEntity() const;

        virtual bool IsValidKnownEntity( CBaseEntity* pEnt ) const;

        virtual void OnHeardSound( CSound* pSound ) OVERRIDE;

        template<typename EachFunc>
        void IterateKnown( EachFunc func ) const
        {
            FOR_EACH_VEC( m_vKnownEnts, i )
            {
                if ( m_vKnownEnts[i]->GetEntity() && func( (const KnownEntity*)m_vKnownEnts[i] ) )
                {
                    return;
                }
            }
        }

    protected:
        virtual bool ShouldUpdateVision();
        virtual bool ShouldUpdateHearing();

        virtual void UpdateHearing();

        virtual void UpdateVision();
        virtual void FindNewEntities( CUtlVector<CBaseEntity*>& vListEnts );

        virtual bool ShouldForgetEntity( const KnownEntity* pKnown ) const;
        virtual void UpdateVisionForKnown( KnownEntity* pKnown );

        virtual bool CanSeeCharacter( CBaseEntity* pEnt ) const;

        KnownEntity* FindKnownOf( CBaseEntity* pEnt ) const;


        CUtlVector<KnownEntity*> m_vKnownEnts;
        float m_flFovCos;

        CountdownTimer m_NextVisionTimer;
        CountdownTimer m_NextHearingTimer;
    };
}
