#include "cbase.h"
#include "animation.h"
#include <tier0/vprof.h>

#include "zmr_zombieanimstate.h"


#ifdef CLIENT_DLL
ConVar zm_debug_zombiehitboxes_client( "zm_debug_zombiehitboxes_client", "0", FCVAR_CHEAT );
#else
ConVar zm_debug_zombiehitboxes_server( "zm_debug_zombiehitboxes_server", "0", FCVAR_CHEAT );
#endif

ConVar zm_sv_debug_drawanimstateinfo( "zm_sv_debug_drawanimstateinfo", "-1", FCVAR_REPLICATED, "Entity index of the zombie to print anim state info of." );


#define MOVELAYER_FADE_RATE         1.0f

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

int CZMZombieAnimState::AnimEventToActivity( ZMZombieAnimEvent_t iEvent, int nData )
{
    switch ( iEvent )
    {
    case ZOMBIEANIMEVENT_IDLE : return ACT_IDLE;
    case ZOMBIEANIMEVENT_ATTACK : return ACT_MELEE_ATTACK1;
    case ZOMBIEANIMEVENT_SWAT : return nData;
    case ZOMBIEANIMEVENT_BANSHEEANIM : return nData;
    default : break;
    }

    return ACT_INVALID;
}

ZMZombieAnimEvent_t CZMZombieAnimState::ActivityToAnimEvent( int iActivity, int& nData )
{
    switch ( iActivity )
    {
    case ACT_IDLE : return ZOMBIEANIMEVENT_IDLE;
    case ACT_MELEE_ATTACK1 : return ZOMBIEANIMEVENT_ATTACK;
    case ACT_ZOM_SWATLEFTLOW :
    case ACT_ZOM_SWATLEFTMID :
    case ACT_ZOM_SWATRIGHTLOW :
    case ACT_ZOM_SWATRIGHTMID :
        nData = iActivity; return ZOMBIEANIMEVENT_SWAT;
    case ACT_FASTZOMBIE_FRENZY :
    case ACT_FASTZOMBIE_BIG_SLASH :
    case ACT_FASTZOMBIE_LAND_RIGHT :
    case ACT_FASTZOMBIE_LAND_LEFT :
    case ACT_FASTZOMBIE_LEAP_STRIKE :
    case ACT_RANGE_ATTACK1 :
    case ACT_HOVER :
    case ACT_HOP :
        nData = iActivity; return ZOMBIEANIMEVENT_BANSHEEANIM;
    default : break;
    }

    return ZOMBIEANIMEVENT_MAX;
}

bool CZMZombieAnimState::DoAnimationEvent( ZMZombieAnimEvent_t iEvent, int nData )
{
    CZMBaseZombie* pOuter = GetOuter();
    bool ret = false;


    int act = AnimEventToActivity( iEvent, nData );
    if ( act != ACT_INVALID )
    {
        ret = pOuter->SetActivity( (Activity)act );
    }
    else
    {
        ret = HandleAnimEvent( iEvent, nData );
    }

    return ret;
}

bool CZMZombieAnimState::HandleAnimEvent( ZMZombieAnimEvent_t iEvent, int nData )
{
    switch ( iEvent )
    {
    // We're burning, change idle & moving activities.
    case ZOMBIEANIMEVENT_ON_BURN :
        if ( GetOuter()->HasActivity( ACT_IDLE_ON_FIRE ) )
            SetIdleActivity( ACT_IDLE_ON_FIRE );

        if ( GetOuter()->HasActivity( ACT_WALK_ON_FIRE ) )
            SetMoveActivity( ACT_WALK_ON_FIRE );
        return true;
    // We were extinguished, play normal activities.
    case ZOMBIEANIMEVENT_ON_EXTINGUISH :
        SetIdleActivity( ACT_IDLE );
        SetMoveActivity( ACT_WALK );
        return true;
    default : break;
    }

    return false;
}

bool CZMZombieAnimState::InitParams()
{
    if ( m_bReady ) return true;


    m_actMove = ACT_WALK;
    m_flMoveActSpeed = 1.0f;

    m_actIdle = ACT_IDLE;


    m_bReady = true;
    return true;
}

void CZMZombieAnimState::Update()
{
    VPROF( "CZMZombieAnimState::Update" );


    if ( !ShouldUpdate() )
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
#else
    GetOuter()->DispatchAnimEvents();
#endif

    ShowDebugInfo();
}

bool CZMZombieAnimState::ShouldUpdate() const
{
    CZMBaseZombie* pOuter = GetOuter();
    if ( !pOuter )
        return false;

    if ( !pOuter->IsAlive() )
        return false;

    return true;
}

void CZMZombieAnimState::ShowDebugInfo()
{
    int index = zm_sv_debug_drawanimstateinfo.GetInt();
    if ( index == -1 )
        return;


    if ( index == GetOuter()->entindex() )
        DrawAnimStateInfo();
}

extern void Anim_StatePrintf( int iLine, const char *pMsg, ... );

void CZMZombieAnimState::DrawAnimStateInfo()
{
    const bool bIsServer =
#ifdef CLIENT_DLL
        CBasePlayer::GetLocalPlayer()->IsServer();
#else
        true;
#endif

    CStudioHdr* hdr = GetOuter()->GetModelPtr();

    //const char* rowbreak = "---------------------------------------------------------------";
    int iLine = 0;
    const int nClientLines = 7;

    if ( bIsServer )
        iLine = nClientLines;

    Anim_StatePrintf( iLine++, "%s: Main: %s (%i) | Cycle: %.2f",
        bIsServer ? "Server" : "Client",
        GetSequenceName( hdr, GetOuter()->GetSequence() ),
        GetOuter()->GetSequence(),
        GetOuter()->GetCycle() );
    //Anim_StatePrintf( iLine++, rowbreak );

    for ( int i = 0; i < m_vOverlays.Count(); i++ )
    {
        CZMAnimOverlay* pOverlay = &m_vOverlays[i];

        Anim_StatePrintf( iLine++, "%i | Sequence: %s (%i) | Weight: %.2f",
            pOverlay->GetLayerIndex(),
            GetSequenceName( hdr, pOverlay->GetLayerSequence() ),
            pOverlay->GetLayerSequence(),
            pOverlay->GetLayerWeight() );
    }

#ifdef CLIENT_DLL
    //Anim_StatePrintf( iLine++, rowbreak );
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

int CZMZombieAnimState::GetOuterRandomSequence( Activity act ) const
{
    CZMBaseZombie* pOuter = GetOuter();
    CStudioHdr* hdr = pOuter->GetModelPtr();

    VerifySequenceIndex( hdr );
    
    RandomSeed( pOuter->GetAnimationRandomSeed() ); // Don't use random->SetSeed, SelectWeightedSequence does not use it.
    int iSeq = hdr->SelectWeightedSequence( act, pOuter->GetSequence() );

    return iSeq;
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
                float newweight = pOver->GetLayerWeight() - pOuter->GetAnimTimeInterval() * pOver->GetKillRate();
                pOver->SetLayerWeight( clamp( newweight, 0.0f, 1.0f ) );
            }
            else
            {
                if ( FastRemoveLayer( pOver->GetUniqueId() ) )
                    --i;

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

    for ( int i = 0; i < m_vOverlays.Count(); i++ )
    {
        CZMAnimOverlay* pOver = &m_vOverlays[i];

        if ( !pOver->IsUsed() )
        {
            m_vOverlays.Remove( i );
            --i;
            continue;
        }


        if ( pOver->IsDying() && pOver->GetLayerWeight() <= 0.0f )
        {
            if ( FastRemoveLayer( pOver->GetUniqueId() ) )
                --i;
        }
    }
#endif
}

int CZMZombieAnimState::AddLayeredSequence( int iSeq, int priority )
{
    CZMBaseZombie* pOuter = GetOuter();

    // Take over an old layer that is dying if it's the same sequence.
    float oldcycle = -1.0f;
    float oldweight = -1.0f;
    int i = -1;
    while ( (i = FindLayerBySeq( iSeq, ++i )) != -1 )
    {
        if ( !m_vOverlays[i].IsUsed() )
            continue;

        if ( m_vOverlays[i].IsDying() )
        {
            oldcycle = m_vOverlays[i].GetLayerCycle();
            oldweight = m_vOverlays[i].GetLayerWeight();

            m_vOverlays[i].FastRemove();
            m_vOverlays.Remove( i );
            --i;
        }
    }
    
    int index = m_vOverlays.AddToTail();

    m_vOverlays[index].Create( pOuter, iSeq, priority );

    m_vOverlays[index].SetUniqueId( m_iNextId++ );


    if ( oldcycle >= 0.0f )
    {
        m_vOverlays[index].SetLayerCycle( oldcycle );
    }
    if ( oldweight >= 0.0f )
    {
        m_vOverlays[index].SetLayerWeight( oldweight );
    }

    return m_vOverlays[index].GetUniqueId();
}

void CZMZombieAnimState::RemoveLayer( int id, float rate, float delay )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;

    m_vOverlays[index].Remove( rate, delay );
}

bool CZMZombieAnimState::FastRemoveLayer( int id )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return false;


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

    return true;
}

float CZMZombieAnimState::GetLayerCycle( int id )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return -1.0f;

    return m_vOverlays[index].GetLayerCycle();
}

void CZMZombieAnimState::SetLayerCycle( int id, float cycle )
{
    int index = FindLayerById( id );
    if ( index == -1 ) return;

    m_vOverlays[index].SetLayerCycle( cycle );
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

int CZMZombieAnimState::FindLayerBySeq( int iSeq, int startindex ) const
{
    int len = m_vOverlays.Count();
    for ( int i = MAX( startindex, 0 ); i < len; i++ )
    {
        if ( m_vOverlays[i].GetLayerSequence() == iSeq )
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


    bool bMoving = spdSqr2d > 8.0f; // Only consider XY velocity for moving.


    if ( bMoving && !m_bWasMoving ) // Just started moving
    {
        if ( m_iMoveLayer != -1 )
        {
            FastRemoveLayer( m_iMoveLayer );
            m_iMoveLayer = -1;
        }


        int iSeq = GetOuterRandomSequence( m_actIdle );
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
            RemoveLayer( m_iMoveLayer, MOVELAYER_FADE_RATE );
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


    Activity oldAct = m_actMove;

    m_actMove = act;


    if ( GetOuterActivity() == oldAct )
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
