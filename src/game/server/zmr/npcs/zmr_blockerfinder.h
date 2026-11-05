#pragma once


#include "npcr_component.h"
#include "zmr_zombiebase.h"

//class CZMBaseZombie;

class CZMBlockerScanner : public NPCR::CEventListener
{
public:
    typedef NPCR::CEventListener BaseClass;

    CZMBlockerScanner( CZMBaseZombie* pChar );
    ~CZMBlockerScanner();


    virtual const char* GetComponentName() const override { return "BlockerScanner"; }


    virtual void Update() override;


    virtual CZMBaseZombie* GetOuter() const override { return static_cast<CZMBaseZombie*>( BaseClass::GetOuter() ); }


    int             GetTimesBlocked() const { return m_nTimesBlocked; }
    CBaseEntity*    GetBlocker() const { return m_hBlocker.Get(); }
    void            ClearBlocker() { m_hBlocker.Set( nullptr ); }
    void            ClearTimesBlocked() { m_nTimesBlocked = 0; }

private:
    void FindBlocker();
    bool ShouldUpdateBlocker() const;

    CountdownTimer m_NextBlockerCheck;
    int m_nTimesBlocked;
    CHandle<CBaseEntity> m_hBlocker;
};
