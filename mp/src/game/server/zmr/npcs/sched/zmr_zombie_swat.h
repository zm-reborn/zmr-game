#pragma once

#include "npcs/zmr_zombiebase.h"


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
    Activity m_SwatAct;
    bool m_bDidSwat;
    CHandle<CBaseEntity> m_hSwatObject;
public:
    virtual const char* GetName() const OVERRIDE { return "ZombieSwatObj"; }


    virtual void OnStart() OVERRIDE
    {
        m_SwatAct = ACT_INVALID;
        m_bDidSwat = false;


        CZMBaseZombie* pOuter = GetOuter();

        CBaseEntity* pSwat = pOuter->GetSwatObject();
        if ( !pSwat )
        {
            End( "No object to swat!" );
            return;
        }


        m_hSwatObject.Set( pSwat );

        m_SwatAct = pOuter->GetSwatActivity( pSwat, DoBreakObject() );
        if ( !pOuter->DoAnimationEvent( ZOMBIEANIMEVENT_SWAT, m_SwatAct ) )
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

            m_bDidSwat = true;
        }
    }

    // Wait for us to finish before doing something else.
    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return NPCR::RES_YES;
    }

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        return NPCR::RES_NO;
    }

    virtual void OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com ) OVERRIDE
    {
        // ZM wants us to do something else.

        // Allow by default if we finished swatting.
        if ( !m_bDidSwat && pPlayer )
        {
            // Check if ZM wants for our swat to be interrupted.
            int flags = ToZMPlayer( pPlayer )->GetZMCommandInterruptFlags();

            if ( !(flags & ZCO_SWAT) )
                return;
        }


        TryEnd( "We were commanded to do something else!" );

        if ( GetOuter()->GetActivity() == m_SwatAct )
        {
            GetOuter()->DoAnimationEvent( ZOMBIEANIMEVENT_IDLE );
        }
    }

    void DoSwatting()
    {
        CZMBaseZombie* pOuter = GetOuter();
        if ( pOuter->SwatObject( m_hSwatObject.Get() ) )
        {
            pOuter->EmitSound( "NPC_BaseZombie.Swat" );
        }
    }
};
