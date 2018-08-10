#pragma once

#ifndef CLIENT_DLL
#include "npcr_component.h"
#endif
#include "zmr/npcs/zmr_zombiebase_shared.h"


enum ZMZombieAnimEvent_t
{
    ZOMBIEANIMEVENT_IDLE = 0,

    ZOMBIEANIMEVENT_ATTACK,

    ZOMBIEANIMEVENT_SWAT,

    ZOMBIEANIMEVENT_BANSHEEANIM,

    ZOMBIEANIMEVENT_ON_BURN,
    ZOMBIEANIMEVENT_ON_EXTINGUISH,

    ZOMBIEANIMEVENT_MAX
};

class CZMAnimOverlay
{
public:
    /*
    CZMAnimOverlay()
    {
        m_iLayerIndex = -1;
        m_bKillMe = false;
    }
    */
    void Create( CZMBaseZombie* pOuter, int iSeq, int iPriority )
    {
        m_pOuter = pOuter;

#ifdef GAME_DLL
        m_iLayerIndex = pOuter->AddLayeredSequence( iSeq, iPriority );
#else
        int index = pOuter->m_AnimOverlay.AddToTail();

        C_AnimationLayer* overlay = &pOuter->m_AnimOverlay[index];

        overlay->m_nOrder = 0;
        overlay->m_nSequence = iSeq;
        overlay->m_flWeight = 1.0f;
        overlay->m_bClientBlend = false;
        overlay->m_flCycle = 0.0f;
        overlay->m_flPrevCycle = 0.0f;
        overlay->m_flPlaybackRate = 1.0f;

        m_iLayerIndex = index;
#endif
        m_bKillMe = false;
    }

    void FastRemove()
    {
        if ( !IsUsed() )
            return;

#ifdef GAME_DLL
        m_pOuter->FastRemoveLayer( m_iLayerIndex );
#else
        m_pOuter->m_AnimOverlay.Remove( m_iLayerIndex );
#endif
        m_iLayerIndex = -1;
    }

    void Remove( float rate, float delay = 0.0f )
    {
        if ( !IsUsed() )
            return;

#ifdef GAME_DLL
        m_pOuter->RemoveLayer( m_iLayerIndex, rate, delay );
#endif

        m_bKillMe = true;

#ifdef CLIENT_DLL
        m_flKillRate = m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight / rate;
        m_flKillDelay = delay;
#endif
    }

    int GetUniqueId() const { return m_iId; }

    void SetUniqueId( int id )
    {
        m_iId = id;
    }

    float GetLayerWeight() const
    {
#ifdef CLIENT_DLL
        return m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight;
#else
        return 0.0f;
#endif
    }

    void SetLayerWeight( float flWeight )
    {
#ifdef CLIENT_DLL
        m_pOuter->m_AnimOverlay[m_iLayerIndex].m_flWeight = flWeight;
#else
        m_pOuter->SetLayerWeight( m_iLayerIndex, flWeight );
#endif
    }

    void SetLayerLooping( bool bLoop )
    {
#ifdef CLIENT_DLL
#else
        m_pOuter->SetLayerLooping( m_iLayerIndex, bLoop );
#endif
    }

    bool IsDying() const { return m_bKillMe; }
    
    bool IsUsed() const
    {
        return m_iLayerIndex != -1;
    }

    int GetLayerIndex() const { return m_iLayerIndex; }

    void SetLayerIndex( int index )
    {
        m_iLayerIndex = index;
    }

#ifdef CLIENT_DLL
    float GetKillRate() const { return m_flKillRate; }
    float GetKillDelay() const { return m_flKillDelay; }
    void SetKillDelay( float delay ) { m_flKillDelay = delay; }
#endif

private:
#ifdef CLIENT_DLL
    float m_flKillRate;
    float m_flKillDelay;
#endif
    CZMBaseZombie* m_pOuter;
    int m_iLayerIndex;
    bool m_bKillMe;

    int m_iId;
};


class CZMZombieAnimState
#ifndef CLIENT_DLL
    : public NPCR::CEventListener
#endif
{
public:
#ifndef CLIENT_DLL
    typedef CZMZombieAnimState ThisClass;
    typedef NPCR::CEventListener BaseClass;
#endif

    CZMZombieAnimState( CZMBaseZombie* pNPC );
    ~CZMZombieAnimState();

#ifndef CLIENT_DLL
    virtual const char* GetComponentName() const OVERRIDE { return "ZombieAnimState"; }

    virtual CZMBaseZombie* GetOuter() const OVERRIDE { return static_cast<CZMBaseZombie*>( BaseClass::GetOuter() ); }
#else
    CZMBaseZombie* GetOuter() const { return m_pOuter; }
#endif

    Activity    GetOuterActivity() const;
    bool        SetOuterActivity( Activity act ) const;

    Vector      GetOuterVelocity() const;
    int         GetOuterRandomSequence( Activity act ) const;


    static int                  AnimEventToActivity( ZMZombieAnimEvent_t iEvent, int nData = 0 );
    static ZMZombieAnimEvent_t  ActivityToAnimEvent( int iActivity, int& nData );

    virtual bool HandleAnimEvent( ZMZombieAnimEvent_t iEvent, int nData );

    bool DoAnimationEvent( ZMZombieAnimEvent_t iEvent, int nData );

    void UpdateLayers();


    int FindLayerById( int id ) const;


    CUtlVector<CZMAnimOverlay> m_vOverlays;


    virtual void Update();

    virtual void UpdateMovement();

    //virtual void OnNewActivity();

    Activity    GetMoveActivity() const { return m_actMove; }
    void        SetMoveActivity( Activity act );

    Activity    GetIdleActivity() const { return m_actIdle; }
    void        SetIdleActivity( Activity act );

    int         GetCurrentMoveSequence() const { return m_iMoveSeq; }

    void        UpdateMoveActivity();

protected:
    virtual bool InitParams();
    virtual bool ShouldUpdate() const;


    virtual void    ShowDebugInfo();
    void            DrawAnimStateInfo();



    int     AddLayeredSequence( int iSeq, int priority );
    void    RemoveLayer( int id, float rate = 0.2f, float delay = 0.0f );
    void    FastRemoveLayer( int id );
    void    SetLayerLooping( int id, bool bLoop );
    void    SetLayerWeight( int id, float flWeight );

private:
    int m_iMoveLayer;

    bool m_bWasMoving;
    float m_flMoveWeight;
    float m_flMoveActSpeed;
    float m_flLastSpdSqr;

    Activity m_actMove;
    Activity m_actIdle;

    int m_iMoveSeq;

    bool m_bReady;

    int m_iNextId;

    CZMAnimOverlay m_Overlays[8];
    
#ifdef CLIENT_DLL
    CZMBaseZombie* m_pOuter;
#endif
};
