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

    m_bWasMoving = false;
    m_actIdle = ACT_INVALID;
    m_actMove = ACT_INVALID;
    m_flMoveActSpeed = 0.0f;
    m_flMoveWeight = 0.0f;
    m_flLastSpdSqr = 0.0f;
    m_bReady = false;
    m_iMoveSeq = -1;
    m_iMoveRandomSeed = 0;
}

CZMZombieAnimState::~CZMZombieAnimState()
{

}

int CZMZombieAnimState::AnimEventToActivity( ZMZombieAnimEvent_t iEvent, int nData )
{
    switch ( iEvent )
    {
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
    case ACT_GESTURE_FLINCH_CHEST :
    case ACT_GESTURE_FLINCH_LEFTARM :
    case ACT_GESTURE_FLINCH_RIGHTARM :
        nData = iActivity; return ZOMBIEANIMEVENT_GESTURE;
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
    case ZOMBIEANIMEVENT_IDLE :
        SetOuterActivity( ACT_IDLE );
        m_iMoveRandomSeed = nData; // Use the data as seed in the future.
        return true;
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
    case ZOMBIEANIMEVENT_GESTURE :
        AddLayeredSequence( GetOuterRandomSequence( (Activity)nData ), ANIMOVERLAY_SLOT_GESTURE );
        SetLayerLooping( ANIMOVERLAY_SLOT_GESTURE, false );
        SetLayerCycle( ANIMOVERLAY_SLOT_GESTURE, 0.0f );
        SetLayerWeight( ANIMOVERLAY_SLOT_GESTURE, 1.0f );
        return true;
    default : break;
    }

    return false;
}

bool CZMZombieAnimState::InitParams()
{
    if ( m_bReady ) return true;


    CZMBaseZombie* pOuter = GetOuter();
    pOuter->SetNumAnimOverlays( ANIMOVERLAY_SLOT_MAX );
#ifdef CLIENT_DLL
    // Apparently the constructor doesn't set the order...
    for ( int i = 0; i < pOuter->m_AnimOverlay.Count(); i++ )
    {
        pOuter->m_AnimOverlay[i].m_nOrder = i;
    }
#endif

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


    // Update the actual move animation
    UpdateMovement();
    // Updates our overlay list, and weight and cycle on client
    UpdateLayers();

    // Finally, advance our animations
    UpdateOuterAnims();


    ShowDebugInfo();
}

void CZMZombieAnimState::UpdateOuterAnims()
{
    VPROF_BUDGET( "CZMZombieAnimState::UpdateOuterAnims", "NPCR" );

#ifndef CLIENT_DLL
    // Advance weight and cycle
    GetOuter()->StudioFrameAdvance();
    GetOuter()->DispatchAnimEvents( GetOuter() );
#else
    GetOuter()->DispatchAnimEvents();
#endif
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


    int index = zm_sv_debug_drawanimstateinfo.GetInt();
    if ( index > -1 && index == GetOuter()->entindex() )
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

    int iLine = 0;
    const int nClientLines = 7;

    if ( bIsServer )
        iLine = nClientLines;

    Anim_StatePrintf( iLine++, "%s: Main: %s (%i) | Cycle: %.2f",
        bIsServer ? "Server" : "Client",
        GetSequenceName( hdr, GetOuter()->GetSequence() ),
        GetOuter()->GetSequence(),
        GetOuter()->GetCycle() );


    for ( int i = 0; i < ARRAYSIZE( m_AnimOverlay ); i++ )
    {
        CZMAnimOverlay* pOverlay = &m_AnimOverlay[i];

        if ( !pOverlay->IsUsed() )
            continue;


        Anim_StatePrintf( iLine++, "%i | Sequence: %s (%i) | Weight: %.2f | Cycle: %.2f",
            pOverlay->GetLayerIndex(),
            GetSequenceName( hdr, pOverlay->GetLayerSequence() ),
            pOverlay->GetLayerSequence(),
            pOverlay->GetLayerWeight(),
            pOverlay->GetLayerCycle() );
    }
}

Activity CZMZombieAnimState::GetOuterActivity() const
{
    return GetOuter()->GetActivity();
}

bool CZMZombieAnimState::SetOuterActivity( Activity act ) const
{
    return GetOuter()->SetActivity( act );
}

void CZMZombieAnimState::SetOuterCycle( float cycle ) const
{
    GetOuter()->SetCycle( cycle );
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
    // We have to update layers manually on the client.


    // Update dying/unused overlays.
    for ( int i = 0; i < ARRAYSIZE( m_AnimOverlay ); i++ )
    {
        CZMAnimOverlay* pOver = &m_AnimOverlay[i];

        if ( !pOver->IsUsed() )
        {
            continue;
        }


        if ( pOver->IsDying() )
        {
            // Update the weight until it hits 0 and remove it
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
                FastRemoveLayer( (ZMAnimLayerSlot_t)i );
                continue;
            }
        }
    }

    // Update cycle
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
            animlayer->m_flCycle = 0.0f;


            ZMAnimLayerSlot_t slot = FindSlotByLayerIndex( i );
            if ( slot == ZMAnimLayerSlot_t::ANIMOVERLAY_SLOT_MAX )
            {
                continue;
            }


            if ( !IsLayerLooping( slot ) )
            {
                FastRemoveLayer( slot );
            }
        }
    }
#endif
}

void CZMZombieAnimState::AddLayeredSequence( int iSeq, ZMAnimLayerSlot_t index )
{
    CZMBaseZombie* pOuter = GetOuter();

    // Take over an old layer that is dying if it's the same sequence.
    float oldcycle = -1.0f;
    float oldweight = -1.0f;
    if ( m_AnimOverlay[index].IsUsed() && m_AnimOverlay[index].GetLayerSequence() == iSeq )
    {
        oldcycle = m_AnimOverlay[index].GetLayerCycle();
        oldweight = m_AnimOverlay[index].GetLayerWeight();
    }

    m_AnimOverlay[index].Create( pOuter, iSeq, index );


    if ( oldcycle >= 0.0f )
    {
        m_AnimOverlay[index].SetLayerCycle( oldcycle );
    }
    if ( oldweight >= 0.0f )
    {
        m_AnimOverlay[index].SetLayerWeight( oldweight );
    }
}

void CZMZombieAnimState::RemoveLayer( ZMAnimLayerSlot_t index, float rate, float delay )
{
    m_AnimOverlay[index].Remove( rate, delay );
}

void CZMZombieAnimState::FastRemoveLayer( ZMAnimLayerSlot_t index )
{
    m_AnimOverlay[index].FastRemove();
}

bool CZMZombieAnimState::HasLayeredSequence( ZMAnimLayerSlot_t index ) const
{
    return m_AnimOverlay[index].IsUsed();
}

bool CZMZombieAnimState::IsLayerDying( ZMAnimLayerSlot_t index ) const
{
    return m_AnimOverlay[index].IsDying();
}

bool CZMZombieAnimState::IsLayerLooping( ZMAnimLayerSlot_t index ) const
{
    return m_AnimOverlay[index].IsLayerLooping();
}

float CZMZombieAnimState::GetLayerCycle( ZMAnimLayerSlot_t index )
{
    return m_AnimOverlay[index].GetLayerCycle();
}

void CZMZombieAnimState::SetLayerCycle( ZMAnimLayerSlot_t index, float cycle )
{
    m_AnimOverlay[index].SetLayerCycle( cycle );
}

void CZMZombieAnimState::SetLayerLooping( ZMAnimLayerSlot_t index, bool bLoop )
{
    m_AnimOverlay[index].SetLayerLooping( bLoop );
}

void CZMZombieAnimState::SetLayerWeight( ZMAnimLayerSlot_t index, float flWeight )
{
    m_AnimOverlay[index].SetLayerWeight( flWeight );
}

void CZMZombieAnimState::UpdateMovement()
{
    // Update the idle <-> move animation layer transition to have the natural appearance of acceleration and deceleration.




    CZMBaseZombie* pOuter = GetOuter();

    Activity iCurAct = GetOuterActivity();

    // If some other activity is currently playing, ignore us.
    if ( iCurAct != m_actMove && iCurAct != m_actIdle && iCurAct != ACT_INVALID )
    {
        // The activity was finished, idle.
        if ( pOuter->IsSequenceFinished() && !pOuter->SequenceLoops() )
        {
            SetOuterActivity( m_actIdle );
        }
        else
        {
            // The activity is still going, remove our layer.
            if ( HasLayeredSequence( ANIMOVERLAY_SLOT_IDLE ) )
            {
                FastRemoveLayer( ANIMOVERLAY_SLOT_IDLE );
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
        int iSeq = GetOuterRandomSequence( m_actIdle );
        if ( iSeq >= 0 )
            AddLayeredSequence( iSeq, ANIMOVERLAY_SLOT_IDLE );


        SetLayerLooping( ANIMOVERLAY_SLOT_IDLE, true );


        UpdateMoveActivity();
    }
    else if ( !bMoving && m_bWasMoving ) // Stopped moving
    {
        SetOuterActivity( m_actIdle );

        if ( HasLayeredSequence( ANIMOVERLAY_SLOT_IDLE ) && !IsLayerDying( ANIMOVERLAY_SLOT_IDLE ) )
        {
            RemoveLayer( ANIMOVERLAY_SLOT_IDLE, MOVELAYER_FADE_RATE );
        }
    }
    else if ( bMoving )
    {
        if ( pOuter->GetActivity() != m_actMove )
            UpdateMoveActivity();
    }


    // Update the layer depending how fast we're going.
    if ( HasLayeredSequence( ANIMOVERLAY_SLOT_IDLE ) && !IsLayerDying( ANIMOVERLAY_SLOT_IDLE ) )
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

            SetLayerWeight( ANIMOVERLAY_SLOT_IDLE, m_flMoveWeight );
        }
        else
        {
            SetLayerWeight( ANIMOVERLAY_SLOT_IDLE, 0.0f );
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


    // Set a random cycle, so we don't look too synchronized
    // with other zombies.
    RandomSeed( m_iMoveRandomSeed );
    float randomcycle = RandomFloat();

    SetOuterCycle( randomcycle );



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

ZMAnimLayerSlot_t CZMZombieAnimState::FindSlotByLayerIndex( int index )
{
    for ( int i = 0; i < ARRAYSIZE( m_AnimOverlay ); i++ )
    {
        if ( !m_AnimOverlay[i].IsUsed() )
            continue;

        if ( m_AnimOverlay[i].GetLayerIndex() == index )
            return (ZMAnimLayerSlot_t)i;
    }

    return ANIMOVERLAY_SLOT_MAX;
}


CZMAnimOverlay::CZMAnimOverlay()
{
    m_pOuter = nullptr;
    m_iLayerIndex = -1;
#ifdef CLIENT_DLL
    m_bKillMe = false;
#endif
}

bool CZMAnimOverlay::IsDying() const
{
#ifdef CLIENT_DLL
    return m_bKillMe;
#else
    return m_iLayerIndex != -1 && m_pOuter->GetAnimOverlay( m_iLayerIndex )->IsKillMe();
#endif
}

bool CZMAnimOverlay::IsUsed() const
{
#ifdef CLIENT_DLL
    return m_iLayerIndex != -1 && m_pOuter->m_AnimOverlay[m_iLayerIndex].IsActive();
#else
    return m_iLayerIndex != -1 && m_pOuter->GetAnimOverlay( m_iLayerIndex )->m_nOrder != CBaseAnimatingOverlay::MAX_OVERLAYS;
#endif
}

void CZMAnimOverlay::Create( CZMBaseZombie* pOuter, int iSeq, ZMAnimLayerSlot_t index )
{
    if ( IsUsed() )
    {
        FastRemove();
    }

    m_pOuter = pOuter;

#ifdef GAME_DLL
    m_iLayerIndex = pOuter->AddLayeredSequence( iSeq, CBaseAnimatingOverlay::MAX_OVERLAYS - 1 - index );
#else
    C_AnimationLayer* overlay = &pOuter->m_AnimOverlay[index];

    overlay->m_nOrder = (int)index;
    overlay->m_nSequence = iSeq;
    overlay->m_flWeight = 1.0f;
    overlay->m_bClientBlend = false;
    overlay->m_flCycle = 0.0f;
    overlay->m_flPrevCycle = 0.0f;
    overlay->m_flPlaybackRate = 1.0f;

    m_iLayerIndex = index;

    m_bKillMe = false;
#endif

    Assert( m_iLayerIndex == index );
}

void CZMAnimOverlay::FastRemove()
{
    if ( !IsUsed() )
        return;

#ifdef GAME_DLL
    m_pOuter->FastRemoveLayer( m_iLayerIndex );
#else
    m_pOuter->m_AnimOverlay[m_iLayerIndex].SetOrder( C_BaseAnimatingOverlay::MAX_OVERLAYS );
    m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight = 0.0f;
#endif

    m_iLayerIndex = -1;
}

void CZMAnimOverlay::Remove( float rate, float delay )
{
    if ( !IsUsed() )
        return;

#ifdef GAME_DLL
    m_pOuter->RemoveLayer( m_iLayerIndex, rate, delay );
#endif

#ifdef CLIENT_DLL
    m_flKillRate = m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight / rate;
    m_flKillDelay = delay;
    m_bKillMe = true;
#endif
}

float CZMAnimOverlay::GetLayerCycle() const
{
#ifdef CLIENT_DLL
    return m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flCycle;
#else
    return m_pOuter->GetAnimOverlay( m_iLayerIndex )->m_flCycle;
#endif
}

void CZMAnimOverlay::SetLayerCycle( float cycle )
{
#ifdef CLIENT_DLL
    m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flCycle = cycle;
#else
    m_pOuter->GetAnimOverlay( m_iLayerIndex )->m_flCycle = cycle;
#endif
}

int CZMAnimOverlay::GetLayerSequence() const
{
#ifdef CLIENT_DLL
    return m_pOuter->m_AnimOverlay[m_iLayerIndex].m_nSequence;
#else
    return m_pOuter->GetAnimOverlay( m_iLayerIndex )->m_nSequence;
#endif
}

float CZMAnimOverlay::GetLayerWeight() const
{
#ifdef CLIENT_DLL
    return m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight;
#else
    return m_pOuter->GetAnimOverlay( m_iLayerIndex )->m_flWeight;
#endif
}

void CZMAnimOverlay::SetLayerWeight( float flWeight )
{
#ifdef CLIENT_DLL
    m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight = flWeight;
#else
    m_pOuter->SetLayerWeight( m_iLayerIndex, flWeight );
#endif
}

bool CZMAnimOverlay::IsLayerLooping() const
{
    return m_bLooping;
}

void CZMAnimOverlay::SetLayerLooping( bool bLoop )
{
    m_bLooping = bLoop;

#ifdef GAME_DLL
    m_pOuter->SetLayerLooping( m_iLayerIndex, bLoop );
#endif
    
}
