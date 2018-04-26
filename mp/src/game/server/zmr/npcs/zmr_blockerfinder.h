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


    virtual const char* GetComponentName() const OVERRIDE { return "BlockerScanner"; }


    virtual void Update() OVERRIDE;


    virtual CZMBaseZombie* GetOuter() const OVERRIDE { return static_cast<CZMBaseZombie*>( BaseClass::GetOuter() ); }


    int             GetTimesBlocked() const { return m_nTimesBlocked; }
    CBaseEntity*    GetBlocker() const { return m_hBlocker.Get(); }
    void            ClearBlocker() { m_hBlocker.Set( nullptr ); }
    void            ClearTimesBlocked() { m_nTimesBlocked = 0; }

private:
    //void FindSwatObj();
    void FindBlocker();

    bool ShouldUpdateBlocker() const;
    //bool ShouldUpdateSwatting() const;

    CZMBaseZombie* m_pChar;

    CountdownTimer m_NextBlockerCheck;
    int m_nTimesBlocked;
    CHandle<CBaseEntity> m_hBlocker;
};
