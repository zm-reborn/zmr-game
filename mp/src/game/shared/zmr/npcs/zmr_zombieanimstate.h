#pragma once

#ifndef CLIENT_DLL
#include "npcr_component.h"
#endif
#include "npcs/zmr_zombiebase_shared.h"


enum ZMZombieAnimEvent_t
{
    ZOMBIEANIMEVENT_IDLE = 0,

    ZOMBIEANIMEVENT_ATTACK,

    ZOMBIEANIMEVENT_SWAT,

    ZOMBIEANIMEVENT_BANSHEEANIM,

    ZOMBIEANIMEVENT_ON_BURN,
    ZOMBIEANIMEVENT_ON_EXTINGUISH,

    ZOMBIEANIMEVENT_GESTURE,


    ZOMBIEANIMEVENT_MAX
};


enum ZMAnimLayerSlot_t
{
    ANIMOVERLAY_SLOT_IDLE = 0,
    ANIMOVERLAY_SLOT_GESTURE,

    ANIMOVERLAY_SLOT_MAX
};


// Acts as a wrapper between the actual animation layer and the anim state.
class CZMAnimOverlay
{
public:
    CZMAnimOverlay();

    void Create( CZMBaseZombie* pOuter, int iSeq, ZMAnimLayerSlot_t index );

    void FastRemove();

    void Remove( float rate, float delay = 0.0f );

    float GetLayerCycle() const;
    void SetLayerCycle( float cycle );

    int GetLayerSequence() const;

    float GetLayerWeight() const;
    void SetLayerWeight( float flWeight );

    bool IsLayerLooping() const;
    void SetLayerLooping( bool bLoop );

    bool IsDying() const;
    bool IsUsed() const;

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
    bool m_bKillMe;
#endif

    bool m_bLooping;
    CZMBaseZombie* m_pOuter;
    int m_iLayerIndex;
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
    void        SetOuterCycle( float cycle ) const;

    Vector      GetOuterVelocity() const;
    int         GetOuterRandomSequence( Activity act ) const;

    int         AnimEventToActivity( ZMZombieAnimEvent_t iEvent, int nData = 0 );

    static ZMZombieAnimEvent_t  ActivityToAnimEvent( int iActivity, int& nData );

    virtual bool HandleAnimEvent( ZMZombieAnimEvent_t iEvent, int nData );

    bool DoAnimationEvent( ZMZombieAnimEvent_t iEvent, int nData );

    void UpdateLayers();
    void UpdateOuterAnims();



    virtual void Update();

    virtual void UpdateMovement();

    //virtual void OnNewActivity();

    Activity    GetMoveActivity() const { return m_actMove; }
    void        SetMoveActivity( Activity act );

    Activity    GetIdleActivity() const { return m_actIdle; }
    void        SetIdleActivity( Activity act );

    int         GetCurrentMoveSequence() const { return m_iMoveSeq; }

    void        UpdateMoveActivity();


    float       CalcMovementPlaybackRate() const;
    float       GetMaxGroundSpeed() const;

    void        OnNewModel();

protected:
    virtual bool InitParams();
    virtual bool ShouldUpdate() const;


    virtual void    ShowDebugInfo();
    void            DrawAnimStateInfo();



    void    AddLayeredSequence( int iSeq, ZMAnimLayerSlot_t index );
    void    RemoveLayer( ZMAnimLayerSlot_t index, float rate = 0.2f, float delay = 0.0f );
    void    FastRemoveLayer( ZMAnimLayerSlot_t index );
    bool    HasLayeredSequence( ZMAnimLayerSlot_t index ) const;
    bool    IsLayerDying( ZMAnimLayerSlot_t index ) const;
    bool    IsLayerLooping( ZMAnimLayerSlot_t index ) const;
    float   GetLayerCycle( ZMAnimLayerSlot_t index );
    void    SetLayerCycle( ZMAnimLayerSlot_t index, float cycle );
    void    SetLayerLooping( ZMAnimLayerSlot_t index, bool bLoop );
    void    SetLayerWeight( ZMAnimLayerSlot_t index, float flWeight );
    ZMAnimLayerSlot_t FindSlotByLayerIndex( int index );

    void UpdateOldAnims();
    void Update9wayAnims();

    bool Uses9WayAnim() const { return m_bIs9Way; }

private:
    int m_iMoveRandomSeed;
    bool m_bWasMoving;
    float m_flMoveWeight;
    float m_flMoveActSpeed;
    float m_flLastSpdSqr;

    Activity m_actMove;
    Activity m_actIdle;

    int m_iMoveSeq;

    bool m_bModelParamsReady;

    bool m_bIs9Way;
    int m_iPoseParamMoveX;
    int m_iPoseParamMoveY;


    CZMAnimOverlay m_AnimOverlay[ANIMOVERLAY_SLOT_MAX];


#ifdef CLIENT_DLL
    CZMBaseZombie* m_pOuter;
#endif
};
