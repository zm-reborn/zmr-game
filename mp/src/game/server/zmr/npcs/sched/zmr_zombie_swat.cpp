#include "cbase.h"
#include "npcevent.h"

#include "npcs/zmr_zombieanimstate.h"
#include "zmr_zombie_swat.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CZombieSwatObjSchedule::OnStart()
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

void CZombieSwatObjSchedule::OnUpdate()
{
    if ( m_FinishTimer.IsElapsed() )
    {
        End( "Successfully finished swatting!" );
        return;
    }


    CZMBaseZombie* pOuter = GetOuter();

    pOuter->GetMotor()->FaceTowards( m_vecFaceTowards );
}

void CZombieSwatObjSchedule::OnAnimEvent( animevent_t* pEvent )
{
    if ( pEvent->event == CZMBaseZombie::AE_ZOMBIE_SWATITEM )
    {
        GetOuter()->HandledAnimEvent();

        DoSwatting();

        m_bDidSwat = true;
    }
}

// Wait for us to finish before doing something else.
NPCR::QueryResult_t CZombieSwatObjSchedule::IsBusy() const
{
    return NPCR::RES_YES;
}

NPCR::QueryResult_t CZombieSwatObjSchedule::ShouldChase( CBaseEntity* pEnemy ) const
{
    return NPCR::RES_NO;
}

void CZombieSwatObjSchedule::OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com )
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

void CZombieSwatObjSchedule::DoSwatting()
{
    CZMBaseZombie* pOuter = GetOuter();

    auto* pSwat = m_hSwatObject.Get();

    if ( !pSwat )
    {
        TryEnd( "Swatting object was removed?" );
        return;
    }

    // Sanity check range just in case.
    // It's possible other zombies have already swatted it
    // out of range.
    float range = pSwat->BoundingRadius() + pOuter->GetClawAttackRange() + 5.0f;

    if ( pSwat->WorldSpaceCenter().DistToSqr( pOuter->GetAttackPos() ) > (range*range) )
    {
        TryEnd( "Swatting object was suddenly too far during the swat event!" );
        return;
    }


    if ( pOuter->SwatObject( pSwat ) )
    {
        pOuter->EmitSound( "NPC_BaseZombie.Swat" );
    }
}
