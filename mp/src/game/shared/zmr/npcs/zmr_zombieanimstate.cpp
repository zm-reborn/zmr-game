#include "cbase.h"
#include <tier0/vprof.h>

#include "zmr_zombieanimstate.h"


#ifdef CLIENT_DLL
ConVar zm_debug_zombiehitboxes_client( "zm_debug_zombiehitboxes_client", "0", FCVAR_CHEAT );
#else
ConVar zm_debug_zombiehitboxes_server( "zm_debug_zombiehitboxes_server", "0", FCVAR_CHEAT );
#endif


CZMZombieAnimState::CZMZombieAnimState( CZMBaseZombie* pZombie )
#ifdef GAME_DLL
    : NPCR::CEventListener( nullptr, pZombie ) // We update manually.
#endif
{
#ifdef CLIENT_DLL
    m_pOuter = pZombie;
#endif

    m_iMoveLayer = -1;
    m_bWasMoving = false;
    m_actIdle = ACT_INVALID;
    m_actMove = ACT_INVALID;
    m_flMoveActSpeed = 0.0f;
    m_flMoveWeight = 0.0f;
    m_flLastSpdSqr = 0.0f;
    m_bReady = false;
    m_iMoveSeq = -1;


    m_iNextId = 0;


    //GetOuter()->SetNumAnimOverlays( 1 );
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
    VPROF( "CZMZombieAnimState::Update" );


    if ( !GetOuter()->IsAlive() )
        return;


    if ( !InitParams() )
    {
        Warning( "Couldn't initialize animation parameters!\n" );
        return;
    }




    const float flDebugDraw =
#ifdef CLIENT_DLL
        zm_debug_zombiehitboxes_client.GetFloat();
#else
        zm_debug_zombiehitboxes_server.GetFloat();
#endif

    if ( flDebugDraw > 0.0f )
    {
#ifdef CLIENT_DLL
        GetOuter()->DrawClientHitboxes( flDebugDraw, true );
#else
        GetOuter()->DrawServerHitboxes( flDebugDraw, true );
#endif
    }


    UpdateMovement();

    UpdateLayers();

#ifndef CLIENT_DLL
    GetOuter()->StudioFrameAdvance();
    GetOuter()->DispatchAnimEvents( GetOuter() );
#endif
}

Activity CZMZombieAnimState::GetOuterActivity() const
{
    return GetOuter()->GetActivity();
}

bool CZMZombieAnimState::SetOuterActivity( Activity act ) const
{
    return GetOuter()->SetActivity( act );
}

Vector CZMZombieAnimState::GetOuterVelocity() const
{
#ifdef CLIENT_DLL
    Vector vel;
    GetOuter()->EstimateAbsVelocity( vel );

    return vel;
#else
    return GetNPC()->GetMotor()->GetVelocity();
#endif
}

void CZMZombieAnimState::UpdateLayers()
{
    CZMBaseZombie* pOuter = GetOuter();


    CStudioHdr* hdr = pOuter->GetModelPtr();
    if ( !hdr )
        return;

#ifdef CLIENT_DLL
    for ( int i = 0; i < m_vOverlays.Count(); i++ )
    {
        CZMAnimOverlay* pOver = &m_vOverlays[i];

        if ( !pOver->IsUsed() )
        {
            m_vOverlays.Remove( i );
            --i;
            continue;
        }


        if ( pOver->IsDying() )
        {
            if ( pOver->GetKillDelay() > 0.0f )
            {
                float newdelay = pOver->GetKillDelay() - gpGlobals->frametime;
                pOver->SetKillDelay( newdelay );
            }
            else if ( pOver->GetLayerWeight() > 0.0f )
            {
                float newweight = pOver->GetLayerWeight() - gpGlobals->frametime;
                pOver->SetLayerWeight( clamp( newweight, 0.0f, 1.0f ) );
            }
            else
            {
                FastRemoveLayer( pOver->GetUniqueId() );
                continue;
            }
        }
    }

    for ( int i = 0; i < pOuter->m_AnimOverlay.Count(); i++ )
    {
        C_AnimationLayer* animlayer = &pOuter->m_AnimOverlay[i];


        // Get the current cycle.
        float flCycle = animlayer->m_flCycle;
        flCycle += pOuter->GetSequenceCycleRate( hdr, animlayer->m_nSequence ) * gpGlobals->frametime * 1.0f * animlayer->m_flPlaybackRate;

        animlayer->m_flPrevCycle = animlayer->m_flCycle;
        animlayer->m_flCycle = flCycle;

        if( flCycle > 1.0f )
        {
            //RunGestureSlotAnimEventsToCompletion( pGesture );
            /*
            if ( pGesture->m_bAutoKill )
            {
                ResetGestureSlot( pGesture->m_iGestureSlot );
                return;
            }
            else*/
            {
                animlayer->m_flCycle = 0.0f;
            }
        }
    }
#else
    /*
    if ( pGesture->m_iActivity != ACT_INVALID && pGesture->m_pAnimLayer->m_nActivity == ACT_INVALID )
    {
        ResetGestureSlot( pGesture->m_iGestureSlot );
    }
    */
#endif
}

int CZMZombieAnimState::AddLayeredSequence( int iSeq, int priority )
{
    CZMBaseZombie* pOuter = GetOuter();
    
    int index = m_vOverlays.AddToTail();

    m_vOverlays[index].Create( pOuter, iSeq, priority );

    m_vOverlays[index].SetUniqueId( m_iNextId++ );

    return m_vOverlays[index].GetUniqueId();
}

void CZMZombieAnimState::RemoveLayer( int id, float rate, float delay )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;

    m_vOverlays[index].Remove( rate, delay );
}

void CZMZombieAnimState::FastRemoveLayer( int id )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;


    int layer = m_vOverlays[index].GetLayerIndex();

    m_vOverlays[index].FastRemove();
    m_vOverlays.Remove( index );



    for ( int i = 0; i < m_vOverlays.Count(); i++ )
    {
        int mylayer = m_vOverlays[i].GetLayerIndex();
        Assert( mylayer != layer );

        if ( mylayer > layer )
        {
            m_vOverlays[i].SetLayerIndex( --mylayer );
        }
    }
}

void CZMZombieAnimState::SetLayerLooping( int id, bool bLoop )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;

    m_vOverlays[index].SetLayerLooping( bLoop );
}

void CZMZombieAnimState::SetLayerWeight( int id, float flWeight )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;

    m_vOverlays[index].SetLayerWeight( flWeight );
}

int CZMZombieAnimState::FindLayerById( int id ) const
{
    int len = m_vOverlays.Count();
    for ( int i = 0; i < len; i++ )
    {
        if ( m_vOverlays[i].GetUniqueId() == id )
            return i;
    }

    return -1;
}

void CZMZombieAnimState::UpdateMovement()
{
    if ( m_iMoveLayer != -1 && FindLayerById( m_iMoveLayer ) == -1 )
    {
        m_iMoveLayer = -1;
    }





    CZMBaseZombie* pOuter = GetOuter();

    Activity iCurAct = GetOuterActivity();

    // If some other activity is currently playing, ignore us.
    if ( iCurAct != m_actMove && iCurAct != m_actIdle )
    {
        // The activity was finished, idle.
        if ( pOuter->IsSequenceFinished() && !pOuter->SequenceLoops() )
        {
            SetOuterActivity( m_actIdle );
        }
        else
        {
            // The activity is still going, remove our layer.
            if ( m_iMoveLayer != -1 )
            {
                FastRemoveLayer( m_iMoveLayer );
                m_iMoveLayer = -1;
            }

            return;
        }
    }


    Vector vel = GetOuterVelocity();
    float spdSqr2d = vel.Length2DSqr();
    float spdSqr = vel.LengthSqr();


    bool bTryingToMove;

#ifdef CLIENT_DLL
    bTryingToMove = false;
#else
    bTryingToMove = GetNPC()->GetMotor()->IsMoving(); // We've attempted to move recently.
#endif
    bool bMoving = bTryingToMove || spdSqr2d > 1.0f; // Only consider XY velocity for moving.


    if ( bMoving && !m_bWasMoving ) // Just started moving
    {
        if ( m_iMoveLayer != -1 )
        {
            FastRemoveLayer( m_iMoveLayer );
            m_iMoveLayer = -1;
        }

        int iSeq = pOuter->SelectWeightedSequence( m_actIdle );
        if ( iSeq >= 0 )
            m_iMoveLayer = AddLayeredSequence( iSeq, 1 );


        SetLayerLooping( m_iMoveLayer, true );


        UpdateMoveActivity();
    }
    else if ( !bMoving && m_bWasMoving ) // Stopped moving
    {
        SetOuterActivity( m_actIdle );

        if ( m_iMoveLayer != -1 )
        {
            RemoveLayer( m_iMoveLayer, 1.0f );
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

            SetLayerWeight( m_iMoveLayer, m_flMoveWeight );
        }
        else
        {
            SetLayerWeight( m_iMoveLayer, 0.0f );
            m_flMoveWeight = 0.0f;
        }
    }

    m_flLastSpdSqr = spdSqr;
    m_bWasMoving = bMoving;
}

void CZMZombieAnimState::UpdateMoveActivity()
{
    CZMBaseZombie* pOuter = GetOuter();

    if ( GetOuterActivity() == m_actMove )
        return;


    if ( !SetOuterActivity( m_actMove ) )
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


    if ( GetOuterActivity() == m_actIdle )
    {
        SetOuterActivity( act );
    }

    m_actIdle = act;
}
