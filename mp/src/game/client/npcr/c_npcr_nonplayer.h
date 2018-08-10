#pragma once

#include "c_basecombatcharacter.h"

#include "npcevent.h"

class C_NPCRNonPlayer : public C_BaseCombatCharacter
{
public:
    C_NPCRNonPlayer();


    virtual bool IsNPCR() const OVERRIDE { return true; }


    // Shared - Implemented in npcr_nonplayer_shared
public:
    virtual int     GetAnimationRandomSeed();

    bool            HasActivity( Activity act );
    Activity        GetActivity() const { return m_iCurActivity; }
    bool            SetActivity( Activity act );
private:
    Activity    m_iCurActivity;
    Activity    m_iLastActivity;
    bool        m_bCurActivityLoops;
public:


    virtual void DispatchAnimEvents();
    virtual void HandleAnimEvent( animevent_t* pEvent );
};
