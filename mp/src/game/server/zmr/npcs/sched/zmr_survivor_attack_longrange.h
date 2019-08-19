#pragma once

#include "zmr/npcs/zmr_playerbot.h"

#include "zmr_survivor_attack.h"
#include "zmr_survivor_attack_closerange.h"

#include "npcr_motor.h"
#include "npcr_path_cost.h"
#include "npcr_path_follow.h"

class SurvivorAttackLongRangeSched : public NPCR::CSchedule<CZMPlayerBot>, public CZMSurvivorAttackInt
{
private:
    NPCR::CFollowNavPath m_Path;
    NPCR::CPathCostGroundOnly m_PathCost;

    SurvivorAttackCloseRangeSched* m_pAttackCloseRangeSched;
public:
    SurvivorAttackLongRangeSched()
    {
        m_pAttackCloseRangeSched = new SurvivorAttackCloseRangeSched;
    }

    ~SurvivorAttackLongRangeSched()
    {
        delete m_pAttackCloseRangeSched;
    }

    virtual const char* GetName() const OVERRIDE { return "SurvivorCombatLongRange"; }

    virtual NPCR::CSchedule<CZMPlayerBot>* CreateFriendSchedule() OVERRIDE { return nullptr; }

    virtual void OnStart() OVERRIDE
    {
        CZMPlayerBot* pOuter = GetOuter();

        if ( !pOuter->EquipWeaponOfType( BOTWEPRANGE_LONGRANGE ) )
        {
            End( "Couldn't equip any long range weapons!" );
            return;
        }
    }

    virtual void OnContinue() OVERRIDE
    {
        End( "Done." );
    }

    virtual void OnUpdate() OVERRIDE
    {
        CZMPlayerBot* pOuter = GetOuter();

        CBaseEntity* pEnemy = GetAttackTarget();
        if ( !pEnemy || !pEnemy->IsAlive() )
        {
            End( "No enemy left!" );
            return;
        }


        if ( !pOuter->WeaponHasAmmo( pOuter->GetActiveWeapon() ) )
        {
            End( "No ammo left!" );
            return;
        }


        if ( pEnemy->GetAbsOrigin().DistTo( pOuter->GetPosition() ) < 128.0f )
        {
            m_pAttackCloseRangeSched->SetAttackTarget( pEnemy );
            Intercept( m_pAttackCloseRangeSched, "Enemy too close!" );
            return;
        }


        Vector vecAimTarget;


        vecAimTarget = pEnemy->WorldSpaceCenter();

        pOuter->GetMotor()->FaceTowards( vecAimTarget );

        if ( pOuter->GetMotor()->IsFacing( vecAimTarget, 1.0f ) && pOuter->GetSenses()->CanSee( vecAimTarget ) )
        {
            pOuter->PressFire1( 0.1f );
        }
    }

    virtual NPCR::QueryResult_t IsBusy() const OVERRIDE
    {
        return m_Path.IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
    }

    virtual NPCR::QueryResult_t ShouldChase( CBaseEntity* pEnemy ) const OVERRIDE
    {
        return IsDone() ? NPCR::RES_NONE : NPCR::RES_NO;
    }
};
