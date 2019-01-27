#pragma once


#include "basecombatcharacter.h"
#include "ai_basenpc.h"

#include "npcr_basenpc.h"
//#include "npcr_motor_nonplayer.h"


namespace NPCR
{
    class CNonPlayerMotor;
}


//
// Unfortunately, we have to leave this outside the namespace, because data desc gets fucked. Trust me, I tried.
//

// An NPC that simulates its own movement and relies on animation activities and events. Ie. like the Source NPCs.
class CNPCRNonPlayer : public CBaseCombatCharacter, public NPCR::CBaseNPC
{
public:
    DECLARE_CLASS( CNPCRNonPlayer, CBaseCombatCharacter );
    DECLARE_DATADESC();

    CNPCRNonPlayer();
    ~CNPCRNonPlayer();

    virtual NPCR::CBaseMotor* CreateMotor() OVERRIDE;

    virtual void PostConstructor( const char* szClassname ) OVERRIDE;
    virtual void Spawn() OVERRIDE;

    virtual void PostUpdate() OVERRIDE;


    virtual CBaseNPC* MyNPCRPointer() OVERRIDE { return this; }
    //virtual bool IsNPC() const OVERRIDE { return true; }
    virtual bool IsTemplate() OVERRIDE { return HasSpawnFlags( SF_NPC_TEMPLATE ); }

    // Just outright remove us.
    virtual bool RemoveNPC() OVERRIDE { UTIL_Remove( this ); return true; }


    virtual void VPhysicsUpdate( IPhysicsObject* pPhys ) OVERRIDE;
    virtual void PerformCustomPhysics( Vector* pNewPosition, Vector* pNewVelocity, QAngle* pNewAngles, QAngle* pNewAngVelocity ) OVERRIDE;


    virtual void HandleAnimEvent( animevent_t* pEvent ) OVERRIDE;

    virtual int OnTakeDamage_Alive( const CTakeDamageInfo& info ) OVERRIDE;

    virtual void Event_Killed( const CTakeDamageInfo& info ) OVERRIDE;


    // Don't mind the odd name, it's to avoid confusion between move activity ground speed and "move speed"
    virtual float GetMoveActivityMovementSpeed();


    virtual bool    IsEnemy( CBaseEntity* pEnt ) const OVERRIDE { return pEnt && pEnt->IsPlayer(); }
    virtual bool    IsTargetedEnemy( CBaseEntity* pEnt ) const OVERRIDE { return pEnt && GetEnemy() == pEnt; }


        

    // We shouldn't send this anim event to base classes.
    void HandledAnimEvent() { m_bHandledAnimEvent = true; }


    CBaseEntity*    GetEnemy() const { return m_hEnemy.Get(); }
    void            SetEnemy( CBaseEntity* pEnt ) { m_hEnemy.Set( pEnt ); }

    virtual void    AcquireEnemy( CBaseEntity* pEnemy );
    virtual void    LostEnemy();


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


protected:
    void NPCThink();

    void SetDefaultEyeOffset();

private:
    CHandle<CBaseEntity> m_hEnemy; // The enemy we're trying to attack.


    bool        m_bHandledAnimEvent;

    Activity    m_iLastLoopActivity; // For making sure we don't call OnActivityFinished multiple times.


public:
    // Outputs
    COutputEvent m_OnDamaged;
    COutputEvent m_OnDamagedByPlayer;
    COutputEvent m_OnHalfHealth;
    COutputEvent m_OnDeath;
    COutputEHANDLE m_OnFoundPlayer;
    COutputEHANDLE m_OnFoundEnemy;
    COutputEvent m_OnLostPlayerLOS;
    COutputEvent m_OnLostEnemyLOS;
    COutputEvent m_OnLostEnemy;
private:
    // Inputs
    void InputSetHealth( inputdata_t &inputdata );

private:
    float m_flLastDamageTime;
    float m_flNextLOSOutputs;
    bool m_bDidSeeEnemyLastTime;
};
