#include "cbase.h"

#include "zmr_zombieanimstate.h"

CZMZombieAnimState::CZMZombieAnimState( CZMBaseZombie* pZombie )
    : NPCR::CEventListener( nullptr, pZombie ) // We update manually.
{
    m_iMoveLayer = -1;
    m_bWasMoving = false;
    m_actIdle = ACT_INVALID;
    m_actMove = ACT_INVALID;
    m_flMoveActSpeed = 0.0f;
    m_flMoveWeight = 0.0f;
    m_flLastSpdSqr = 0.0f;
    m_bReady = false;
    m_iMoveSeq = -1;
}

CZMZombieAnimState::~CZMZombieAnimState()
{

}

bool CZMZombieAnimState::InitParams()
{
    if ( m_bReady ) return true;


    m_actMove = ACT_WALK;
    m_flMoveActSpeed = GetOuter()->GetSequenceGroundSpeed( GetOuter()->SelectWeightedSequence( m_actMove ) );

    m_actIdle = ACT_IDLE;


    m_bReady = true;
    return true;
}

void CZMZombieAnimState::Update()
{
    if ( !GetOuter()->IsAlive() )
        return;


    if ( !InitParams() )
    {
        Warning( "Couldn't initialize animation parameters!\n" );
        return;
    }


    UpdateMovement();

    GetOuter()->StudioFrameAdvance();
    GetOuter()->DispatchAnimEvents( GetOuter() );
}

void CZMZombieAnimState::UpdateMovement()
{
    CZMBaseZombie* pOuter = GetOuter();

    Activity iCurAct = pOuter->GetActivity();

    // If some other activity is currently playing, ignore us.
    if ( iCurAct != m_actMove && iCurAct != m_actIdle )
    {
        // The activity was finished, idle.
        if ( pOuter->IsSequenceFinished() && !pOuter->SequenceLoops() )
        {
            pOuter->SetActivity( m_actIdle );
        }
        else
        {
            // The activity is still going, remove our layer.
            if ( m_iMoveLayer != -1 )
            {
                pOuter->FastRemoveLayer( m_iMoveLayer );
                m_iMoveLayer = -1;
            }

            return;
        }
    }


    float spdSqr2d = GetNPC()->GetMotor()->GetVelocity().AsVector2D().LengthSqr();
    float spdSqr = GetNPC()->GetMotor()->GetVelocity().LengthSqr();
    bool bTryingToMove = GetNPC()->GetMotor()->IsMoving(); // We've attempted to move recently.
    bool bMoving = bTryingToMove || spdSqr2d > 1.0f; // Only consider XY velocity for moving.


    if ( bMoving && !m_bWasMoving ) // Just started moving
    {
        if ( m_iMoveLayer != -1 )
            pOuter->FastRemoveLayer( m_iMoveLayer );


        int iSeq = pOuter->SelectWeightedSequence( m_actIdle );
        if ( iSeq >= 0 )
            m_iMoveLayer = pOuter->AddLayeredSequence( iSeq, 1 );


        pOuter->SetLayerLooping( m_iMoveLayer, true );


        UpdateMoveActivity();
    }
    else if ( !bMoving && m_bWasMoving ) // Stopped moving
    {
        pOuter->SetActivity( m_actIdle );

        if ( m_iMoveLayer != -1 )
        {
            pOuter->RemoveLayer( m_iMoveLayer, 1.0f );
            m_iMoveLayer = -1;
        }
    }
    else if ( bMoving )
    {
        if ( pOuter->GetActivity() != m_actMove )
            UpdateMoveActivity();
    }


    // Update the layer depending how fast we're going.
    if ( m_iMoveLayer != -1 )
    {
        if ( pOuter->GetActivity() == m_actMove )
        {
            float maxSpdSqr = m_flMoveActSpeed;
            maxSpdSqr *= maxSpdSqr;
            maxSpdSqr = MAX( 1.0f, maxSpdSqr );

            spdSqr = MIN( maxSpdSqr, spdSqr );
            spdSqr = MAX( 0.1f, spdSqr );


            float newWeight = 1.0f - spdSqr / maxSpdSqr;

            
            if ( bTryingToMove )
            {
                newWeight = MIN( newWeight, 0.7f );
                //newWeight = MAX( newWeight, 0.1f );
            }
        
            m_flMoveWeight = clamp( newWeight, 0.0f, 1.0f );

            pOuter->SetLayerWeight( m_iMoveLayer, m_flMoveWeight );
        }
        else
        {
            pOuter->SetLayerWeight( m_iMoveLayer, 0.0f );
            m_flMoveWeight = 0.0f;
        }
    }

    m_flLastSpdSqr = spdSqr;
    m_bWasMoving = bMoving;
}

void CZMZombieAnimState::UpdateMoveActivity()
{
    CZMBaseZombie* pOuter = GetOuter();

    if ( pOuter->GetActivity() == m_actMove )
        return;


    if ( !pOuter->SetActivity( m_actMove ) )
        return;


    m_iMoveSeq = pOuter->GetSequence();


    m_flMoveActSpeed = pOuter->GetSequenceGroundSpeed( pOuter->GetSequence() );
}

void CZMZombieAnimState::SetMoveActivity( Activity act )
{
    if ( act == m_actMove ) return;

    CZMBaseZombie* pOuter = GetOuter();
    if ( !pOuter->HasActivity( act ) )
    {
        DevWarning( "Attempted to set invalid move activity (%i)\n", act );
        return;
    }


    m_actMove = act;


    UpdateMoveActivity();
}

void CZMZombieAnimState::SetIdleActivity( Activity act )
{
    if ( act == m_actIdle ) return;


    CZMBaseZombie* pOuter = GetOuter();
    if ( !pOuter->HasActivity( act ) )
    {
        DevWarning( "Attempted to set invalid idle activity (%i)\n", act );
        return;
    }


    if ( pOuter->GetActivity() == m_actIdle )
    {
        pOuter->SetActivity( act );
    }

    m_actIdle = act;
}
