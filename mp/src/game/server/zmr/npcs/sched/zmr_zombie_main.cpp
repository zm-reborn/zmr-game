#include "cbase.h"

#include "npcs/zmr_blockerfinder.h"
#include "zmr_zombie_combat.h"
#include "zmr_zombie_swat_goto.h"
#include "zmr_zombie_main.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_zombie_move_start_areafinddist( "zm_sv_zombie_move_start_areafinddist", "256" );


CZombieMainSchedule::CZombieMainSchedule()
{
    m_pPath = nullptr;
    m_bHasGoal = false;
    m_vecCurrentGoal = vec3_origin;
    m_pGotoSwatSched = new CZombieGotoSwatObjSchedule;
}

CZombieMainSchedule::~CZombieMainSchedule()
{
    delete m_pGotoSwatSched;
    delete m_pPath;
}

NPCR::CSchedule<CZMBaseZombie>* CZombieMainSchedule::CreateFriendSchedule()
{
    return new CZombieCombatSchedule;
}

void CZombieMainSchedule::OnStart()
{
    delete m_pPath;
    m_pPath = GetOuter()->GetFollowPath();
    m_PathCost = *GetOuter()->GetPathCost();
}

//void CZombieMainSchedule::OnEnd()
//{
//    delete m_pPath;
//}


void CZombieMainSchedule::OnUpdate()
{
    CZMBaseZombie* pOuter = GetOuter();

            
    //
    // Update move path.
    // Commanded move / etc.
    //
    if ( m_pPath->IsValid() )
    {
        // See if we have anything blocking us.
        if ( m_BlockerTimer.IsElapsed() && pOuter->GetBlockerFinder()->GetTimesBlocked() > 1 && pOuter->GetBlockerFinder()->GetBlocker() )
        {
            CBaseEntity* pBlocker = pOuter->GetBlockerFinder()->GetBlocker();

            bool bSwat = pOuter->CanSwatPhysicsObjects() && CZMBaseZombie::CanSwatObject( pBlocker );
            bool bBreak = pOuter->CanBreakObject( pBlocker, true );
            if ( bSwat || bBreak )
            {
                pOuter->GetBlockerFinder()->ClearBlocker();
                pOuter->GetBlockerFinder()->ClearTimesBlocked();

                pOuter->SetSwatObject( pBlocker );


                Intercept( m_pGotoSwatSched, "Clear the blocking entity!" );
                return;
            }

            m_BlockerTimer.Start( 0.5f );
        }


        if ( pOuter->CanMove() )
        {
            m_pPath->Update( pOuter );
        }
    }
    //
    // Update defensive mode
    //
    else if ( pOuter->GetZombieMode() == ZOMBIEMODE_DEFEND )
    {
        const Vector defpos = GetGoalPos();

        float goalsqr = zm_sv_defense_goal_tolerance.GetFloat();
        goalsqr *= goalsqr;
        if ( pOuter->GetPosition().DistToSqr( defpos ) > goalsqr )
        {
            CBaseEntity* pEnemy = pOuter->GetEnemy();

            float chasesqr = zm_sv_defense_chase_dist.GetFloat();
            chasesqr *= chasesqr;
            // Command the zombie back to start.
            if ( pEnemy && !ShouldDefendFrom( pEnemy ) )
            {
                Command( nullptr, defpos );
                return;
            }
        }
    }


    //
    // Update our queued commands
    // and pick the ones we will handle.
    //
    CZMCommandBase* pQueued = pOuter->GetCommandQueue()->NextCommand();
    if ( pQueued )
    {
        if ( pOuter->IsBusy() != NPCR::RES_YES )
        {
            bool bRes = true;

            switch ( pQueued->GetCommandType() )
            {
            case COMMAND_MOVE :
                Command( pQueued->GetCommander(), pQueued->GetVectorTarget() );
                break;
            case COMMAND_SWAT :
                CommandSwat( pQueued->GetCommander(), pQueued->GetObjectTarget(), static_cast<CZMCommandSwat*>( pQueued )->BreakObject() );
                break;
            default :
                bRes = false;
                break;
            }

            if ( bRes )
                pOuter->GetCommandQueue()->RemoveCommand( pQueued );
        }
    }
}

void CZombieMainSchedule::OnCommanded( CBasePlayer* pCommander, ZombieCommandType_t type )
{
    // Lose enemy when we're commanded so we don't keep going for them forever.
    GetOuter()->LostEnemy();
}

void CZombieMainSchedule::OnQueuedCommand( CBasePlayer* pPlayer, ZombieCommandType_t com )
{
    m_pPath->Invalidate();
}

void CZombieMainSchedule::OnChase( CBaseEntity* pEnt )
{
    m_pPath->Invalidate();
}

NPCR::QueryResult_t CZombieMainSchedule::ShouldChase( CBaseEntity* pEnemy ) const
{
    CZMBaseZombie* pOuter = GetOuter();

    if ( pOuter->GetZombieMode() == ZOMBIEMODE_AMBUSH )
        return NPCR::RES_NO;

    if ( pOuter->GetZombieMode() == ZOMBIEMODE_DEFEND && !ShouldDefendFrom( pEnemy ) )
    {
        return NPCR::RES_NO;
    }

    return NPCR::RES_NONE;
}

// Wait for us to finish before doing something else.
NPCR::QueryResult_t CZombieMainSchedule::IsBusy() const
{
    return m_pPath->IsValid() ? NPCR::RES_YES : NPCR::RES_NONE;
}

//
// Commanded to move somewhere.
//
bool CZombieMainSchedule::Command( CZMPlayer* pCommander, const Vector& vecPos )
{
    UpdateGoal( vecPos );


    CZMBaseZombie* pOuter = GetOuter();
    //pOuter->SetEnemy( nullptr );
    pOuter->UpdateLastTimeCommanded();


    Vector vecMyPos = pOuter->GetAbsOrigin();

    CNavArea* pStart = pOuter->GetLastKnownArea();
    if ( !pStart ) // We might not have a position yet if we just spawned.
    {
        float flStartFindDist = zm_sv_zombie_move_start_areafinddist.GetFloat();
        flStartFindDist = fabs( flStartFindDist );

        pStart = TheNavMesh->GetNearestNavArea( pOuter->GetPosition(), true, flStartFindDist, true );
    }

    CNavArea* pGoal = TheNavMesh->GetNearestNavArea( vecPos, true, 128.0f, true );
    m_PathCost.SetStepHeight( pOuter->GetMotor()->GetStepHeight() );
    m_PathCost.SetStartPos( vecMyPos, pStart );
    m_PathCost.SetGoalPos( vecPos, pGoal );


    // Set the goal tolerance based on how far the zombie is.
    const float flToleranceMinDist = 32.0f;
    const float flToleranceMaxDist = 192.0f;
    const float flMinTolerance = 6.0f;
    const float flMaxTolerance = 32.0f;

    float tolerance = SimpleSplineRemapValClamped(
        (vecPos - vecMyPos).Length(),
        flToleranceMinDist, flToleranceMaxDist,
        flMinTolerance,
        flMaxTolerance );
            
    m_pPath->SetGoalTolerance( tolerance );


    m_pPath->Compute( vecMyPos, vecPos, pStart, pGoal, m_PathCost );

    if ( m_pPath->IsValid() )
        pOuter->SetCurrentPath( m_pPath );

    pOuter->OnCommanded( pCommander, COMMAND_MOVE );

    return true;
}

//
// Commanded to swat some object.
//
bool CZombieMainSchedule::CommandSwat( CZMPlayer* pCommander, CBaseEntity* pEnt, bool bBreak )
{
    if ( !pEnt )
        return false;


    CZMBaseZombie* pOuter = GetOuter();
    pOuter->UpdateLastTimeCommanded();
            
    pOuter->SetSwatObject( pEnt );



    pOuter->OnCommanded( pCommander, COMMAND_SWAT );


    m_pGotoSwatSched->SetBreakObject( bBreak );
    Intercept( m_pGotoSwatSched, "Commanded to swat this object!" );
    if ( IsIntercepted() )
    {
        // We have no use for the path anymore.
        m_pPath->Invalidate();
    }

    return true;
}

bool CZombieMainSchedule::ShouldDefendFrom( CBaseEntity* pEnemy ) const
{
    float chasesqr = zm_sv_defense_chase_dist.GetFloat();
    chasesqr *= chasesqr;
    return pEnemy && pEnemy->GetAbsOrigin().DistToSqr( GetOuter()->GetPosition() ) < chasesqr;
}

const Vector& CZombieMainSchedule::GetGoalPos()
{
    if ( !m_bHasGoal )
    {
        UpdateGoal( GetNPC()->GetPosition() );
    }

    return m_vecCurrentGoal;
}

void CZombieMainSchedule::UpdateGoal( const Vector& vecPos )
{
    m_bHasGoal = true;
    m_vecCurrentGoal = vecPos;
}
