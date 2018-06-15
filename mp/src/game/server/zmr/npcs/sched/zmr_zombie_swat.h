#pragma once

#include "zmr/npcs/zmr_zombiebase.h"


extern ConVar zm_sv_swatlift;
extern ConVar zm_sv_swatforcemin;
extern ConVar zm_sv_swatforcemax;
extern ConVar zm_sv_swatangvel;


class CZMSwatInt
{
public:
    CZMSwatInt()
    {
        m_bBreakObject = true;
    }

    bool DoBreakObject() const { return m_bBreakObject; }
    void SetBreakObject( bool state ) { m_bBreakObject = state; }
private:
    bool m_bBreakObject;
};

class SwatObjSched : public NPCR::CSchedule<CZMBaseZombie>, public CZMSwatInt
{
private:
    Vector m_vecFaceTowards;
    CountdownTimer m_FinishTimer;
public:
    virtual const char* GetName() const OVERRIDE { return "ZombieSwatObj"; }


    virtual void OnStart() OVERRIDE
    {
        CZMBaseZombie* pOuter = GetOuter();

        CBaseEntity* pSwat = pOuter->GetSwatObject();
        if ( !pSwat )
        {
            End( "No object to swat!" );
            return;
        }


        if ( !pOuter->SetActivity( pOuter->GetSwatActivity( pSwat, DoBreakObject() ) ) )
        {
            End( "Couldn't set swatting activity!" );
            return;
        }

        float wait = pOuter->SequenceDuration();
        pOuter->SetNextAttack( gpGlobals->curtime + wait );
        //pOuter->SetNextMove( gpGlobals->curtime + wait );


        m_FinishTimer.Start( wait );


        CBaseEntity* pEnemy = pOuter->GetEnemy();
        m_vecFaceTowards = (pEnemy && pOuter->GetActivity() != ACT_MELEE_ATTACK1) ? pEnemy->WorldSpaceCenter() : pSwat->WorldSpaceCenter();
    }

    virtual void OnUpdate() OVERRIDE
    {
        if ( m_FinishTimer.IsElapsed() )
        {
            End( "Successfully finished swatting!" );
            return;
        }


        CZMBaseZombie* pOuter = GetOuter();

        pOuter->GetMotor()->FaceTowards( m_vecFaceTowards );
    }

    virtual void OnAnimEvent( animevent_t* pEvent ) OVERRIDE
    {
        if ( pEvent->event == CZMBaseZombie::AE_ZOMBIE_SWATITEM )
        {
            GetOuter()->HandledAnimEvent();

            DoSwatting();
        }
    }

    // Wait for us to finish before doing something else.
    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        return (!IsDone()) ? NPCR::RES_NO : NPCR::RES_NONE;
    }

    void DoSwatting()
    {
        CZMBaseZombie* pOuter = GetOuter();
        if ( pOuter->SwatObject( pOuter->GetSwatObject() ) )
        {
            pOuter->EmitSound( "NPC_BaseZombie.Swat" );
        }
    }
};
