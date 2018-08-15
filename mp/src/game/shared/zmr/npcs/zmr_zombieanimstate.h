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

// Acts as a wrapper between the actual animation layer and the anim state.
class CZMAnimOverlay
{
public:
    void Create( CZMBaseZombie* pOuter, int iSeq, int iPriority );

    void FastRemove();

    void Remove( float rate, float delay = 0.0f );

    int GetUniqueId() const { return m_iId; }
    void SetUniqueId( int id ) { m_iId = id; }

    int GetLayerCycle() const;
    void SetLayerCycle( float cycle );

    int GetLayerSequence() const;

    float GetLayerWeight() const;
    void SetLayerWeight( float flWeight );

    void SetLayerLooping( bool bLoop );

    bool IsDying() const { return m_bKillMe; }
    
    bool IsUsed() const { return m_iLayerIndex != -1; }

    int GetLayerIndex() const { return m_iLayerIndex; }
    void SetLayerIndex( int index ) { m_iLayerIndex = index; }

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

// Similar to CMultiPlayerAnimState, updates animation and animation layers.
// Animations are no longer transmitted to client from the server. Instead, server sends animation events to client.
// Also handles movement <-> idle transitions. This is done both on client & server parallel.
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
    int FindLayerBySeq( int iSeq, int startindex = 0 ) const;


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
    bool    FastRemoveLayer( int id );
    float   GetLayerCycle( int id );
    void    SetLayerCycle( int id, float cycle );
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
