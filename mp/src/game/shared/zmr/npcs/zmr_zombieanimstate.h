#pragma once

#include "npcr_component.h"
#include "zmr/npcs/zmr_zombiebase_shared.h"

class CZMZombieAnimState : public NPCR::CEventListener
{
public:
    typedef CZMZombieAnimState ThisClass;
    typedef NPCR::CEventListener BaseClass;

    CZMZombieAnimState( CZMBaseZombie* pNPC );
    ~CZMZombieAnimState();


    virtual const char* GetComponentName() const OVERRIDE { return "ZombieAnimState"; }

    virtual CZMBaseZombie* GetOuter() const OVERRIDE { return static_cast<CZMBaseZombie*>( BaseClass::GetOuter() ); }


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
};
