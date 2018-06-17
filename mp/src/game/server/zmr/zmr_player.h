#pragma once


#include "hl2/hl2_player.h"

#include "zmr/zmr_player_shared.h"
#include "zmr/zmr_entities.h"
#include "zmr/weapons/zmr_base.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_playerlocaldata.h"
#include "zmr/zmr_playeranimstate.h"


/*
    NOTE: You have to:
    
    Remove LINK_ENTITY_TO_CLASS in hl2mp_player.cpp

    Override CanSprint manually


*/

extern ConVar zm_sv_antiafk;


class CZMBaseZombie;
class CZMBaseWeapon;
class CZMRagdoll;

enum ZMPlayerState_t
{
    // Happily running around in the game.
    ZMSTATE_ACTIVE = 0,
    ZMSTATE_OBSERVER_MODE, // Noclipping around, watching players, etc.

    NUM_ZMSTATE_PLAYER
};

class CZMPlayerStateInfo
{
public:
    ZMPlayerState_t m_iPlayerState;
    const char* m_pStateName;

    void (CZMPlayer::*pfnEnterState)();	// Init and deinit the state.
    void (CZMPlayer::*pfnLeaveState)();

    void (CZMPlayer::*pfnPreThink)(); // Do a PreThink() in this state.
};

class CZMPlayerModelData
{
public:
    CZMPlayerModelData( KeyValues* kv, bool bIsCustom = false )
    {
        Assert( kv );
        m_kvData = kv->MakeCopy();
        m_szModelName = m_kvData->GetString( "model" );

        m_bIsCustom = bIsCustom;
    }

    ~CZMPlayerModelData()
    {
        if ( m_kvData )
            m_kvData->deleteThis();
        m_kvData = nullptr;
    }

    const char* GetModelName() { return m_szModelName; };
    KeyValues*  GetModelData() { return m_kvData; };
    bool        IsCustom() { return m_bIsCustom; };

private:
    const char* m_szModelName;
    KeyValues* m_kvData;
    bool m_bIsCustom;
};

class CZMPlayer : public CHL2_Player
{
public:
    DECLARE_CLASS( CZMPlayer, CHL2_Player )
    DECLARE_SERVERCLASS()
    //DECLARE_PREDICTABLE()
    DECLARE_DATADESC()
    
    CZMPlayer();
    ~CZMPlayer( void );

    
    static CZMPlayer* CreatePlayer( const char* className, edict_t* ed )
    {
        CZMPlayer::s_PlayerEdict = ed;
        return static_cast<CZMPlayer*>( CreateEntityByName( className ) );
    }

    // All valid and precached player models.
    static CUtlVector<CZMPlayerModelData*> m_PlayerModels;


    static void     AddDefaultPlayerModels( KeyValues* kv );
    static int      PrecachePlayerModels( KeyValues* kv );
    virtual void    Precache() OVERRIDE;

    virtual void    Spawn() OVERRIDE;
    virtual void    UpdateOnRemove() OVERRIDE;
    virtual void    PreThink() OVERRIDE;
    virtual void    PostThink() OVERRIDE;
    virtual void    PlayerDeathThink() OVERRIDE;
    virtual bool    ClientCommand( const CCommand& args ) OVERRIDE;
    void            PickDefaultSpawnTeam();
    virtual void    ChangeTeam( int iTeam ) OVERRIDE;
    bool            ShouldSpawn();

    virtual bool            IsValidObserverTarget( CBaseEntity* pEnt ) OVERRIDE;
    virtual CBaseEntity*    FindNextObserverTarget( bool bReverse ) OVERRIDE;

    virtual int     ShouldTransmit( const CCheckTransmitInfo* ) OVERRIDE;

    virtual void    SetAnimation( PLAYER_ANIM playerAnim ) OVERRIDE;
    bool            SetPlayerModel();
    static int      FindPlayerModel( const char* model );
    void            UpdatePlayerFOV();
    KeyValues*      GetPlayerModelData( const char* model );
    void            SetHandsModel( const char* model );
    void            SetHandsData( KeyValues* kv );
    virtual void    CreateViewModel( int index = 0 ) OVERRIDE;


    virtual void    FlashlightTurnOn() OVERRIDE;
    virtual void    FlashlightTurnOff() OVERRIDE;

    virtual bool    BecomeRagdollOnClient( const Vector& force ) OVERRIDE;
    void            CreateRagdollEntity();

    virtual bool Weapon_Lower() OVERRIDE { return false; };
    
    virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize ) OVERRIDE;
    virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon ) OVERRIDE;

    // Lag compensation stuff...
    void FireBullets( const FireBulletsInfo_t& ) OVERRIDE;
    bool WantsLagCompensationOnNPC( const CZMBaseZombie* pZombie, const CUserCmd* pCmd, const CBitVec<MAX_EDICTS>* pEntityTransmitBits ) const;
    void NoteWeaponFired();

    virtual void    DeathSound( const CTakeDamageInfo& info ) OVERRIDE;
    virtual void    Event_Killed( const CTakeDamageInfo& info ) OVERRIDE;
    virtual int     OnTakeDamage( const CTakeDamageInfo& info ) OVERRIDE;
	virtual void    CommitSuicide( bool bExplode = false, bool bForce = false ) OVERRIDE;
	virtual void    CommitSuicide( const Vector &vecForce, bool bExplode = false, bool bForce = false ) OVERRIDE;

    virtual CBaseEntity* EntSelectSpawnPoint( void ) OVERRIDE;

    virtual void    ImpulseCommands() OVERRIDE;
    virtual void    CheatImpulseCommands( int ) OVERRIDE;

    virtual int     GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound ) OVERRIDE;
    void            GiveDefaultItems( void );
    virtual void    EquipSuit( bool bPlayEffects = false ) OVERRIDE;
    virtual void    RemoveAllItems( bool removeSuit ) OVERRIDE;

    virtual void PlayerUse( void ) OVERRIDE;
    //virtual void PlayUseDenySound() OVERRIDE;


    void State_Transition( ZMPlayerState_t newState );
    void State_Enter( ZMPlayerState_t newState );
    void State_Leave();
    void State_PreThink();
    CZMPlayerStateInfo* State_LookupInfo( ZMPlayerState_t state );

    void State_Enter_ACTIVE();
    void State_PreThink_ACTIVE();
    void State_Enter_OBSERVER_MODE();
    void State_PreThink_OBSERVER_MODE();


    virtual bool StartObserverMode( int mode ) OVERRIDE;
    virtual void StopObserverMode( void ) OVERRIDE;

    
    inline bool IsZM() { return GetTeamNumber() == ZMTEAM_ZM; };
    inline bool IsHuman() { return GetTeamNumber() == ZMTEAM_HUMAN; };

    float m_flNextResourceInc;


    void SetBuildSpawn( CZMEntZombieSpawn* pSpawn )
    {
        int index = 0;
        if ( pSpawn )
            index = pSpawn->entindex();


        m_iBuildSpawnIndex = index;
    }

    void SetBuildSpawn( int index )
    {
        m_iBuildSpawnIndex = index;
    }

    inline CZMEntZombieSpawn* GetBuildSpawn() { return dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( m_iBuildSpawnIndex ) ); };
    inline int GetBuildSpawnIndex() { return m_iBuildSpawnIndex; };

    void DeselectAllZombies();


    void SetTeamSpecificProps();
    void PushAway( const Vector& pos, float force );


    // Implemented in zm_player_shared
    bool                HasEnoughResToSpawn( ZombieClass_t zclass );
    bool                HasEnoughRes( int cost );
    int                 GetWeaponSlotFlags();
    int                 GetResources();
    void                IncResources( int res, bool bLimit = false );
    void                SetResources( int res );
    float               GetFlashlightBattery();
    void                SetFlashlightBattery( float battery );
    bool                Weapon_CanSwitchTo( CBaseCombatWeapon* pWeapon ) OVERRIDE;
    Participation_t     GetParticipation();
    virtual void        PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;
    CBaseCombatWeapon*  GetWeaponForAmmo( int iAmmoType );
    virtual Vector      GetAttackSpread( CBaseCombatWeapon* pWeapon, CBaseEntity* pTarget = nullptr ) OVERRIDE;
    virtual Vector      GetAutoaimVector( float flScale ) OVERRIDE;
    void                DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 );
    float               GetAccuracyRatio() const;
    void                UpdateAccuracyRatio();

    KeyValues*          LoadPlayerModels();


    CZMBaseWeapon*  GetWeaponOfHighestSlot();
    CZMBaseWeapon*  GetWeaponOfSlot( const char* szSlotName );
    CZMBaseWeapon*  GetWeaponOfSlot( int slot );
    void            SetWeaponSlotFlags( int flags ) { m_ZMLocal.m_fWeaponSlotFlags = flags; };
    void            AddWeaponSlotFlag( int flag ) { m_ZMLocal.m_fWeaponSlotFlags |= flag; };
    void            RemoveWeaponSlotFlag( int flag ) { m_ZMLocal.m_fWeaponSlotFlags &= ~flag; };

    inline int  GetPickPriority() { return m_nPickPriority; };
    inline void SetPickPriority( int i ) { m_nPickPriority = i; };
    
    inline float    GetLastActivity() { return m_flLastActivity; };
    inline bool     IsCloseToAFK() { return zm_sv_antiafk.GetInt() > 0 && (gpGlobals->curtime - GetLastActivity()) > (zm_sv_antiafk.GetFloat() * 0.8f); };
    inline bool     IsAFK() { return zm_sv_antiafk.GetInt() > 0 && (gpGlobals->curtime - GetLastActivity()) > zm_sv_antiafk.GetFloat(); };

    inline float    GetNextModelChangeTime() { return m_flNextModelChangeTime; };

    int             GetZMCommandInterruptFlags() const;

private:
    // Since I didn't get this at first either, this is only sent to THIS player.
    CNetworkVarEmbedded( CZMPlayerLocalData, m_ZMLocal );

    CNetworkQAngle( m_angEyeAngles );
    CNetworkVar( int, m_iSpawnInterpCounter );
    CNetworkHandle( CZMRagdoll, m_hRagdoll );

    int                 m_iLastWeaponFireUsercmd;
    Vector              m_vecTotalBulletForce; // Accumulator for bullet force in a single frame
	ZMPlayerState_t     m_iPlayerState;
	CZMPlayerStateInfo* m_pCurStateInfo;
    bool                m_bEnterObserver;
    float               m_flNextModelChangeTime;

    CZMPlayerAnimState* m_pPlayerAnimState;

    int m_iBuildSpawnIndex; // To update build menu.
    //Participation_t m_iParticipation;
    int m_nPickPriority;
    float m_flLastActivity;
    float m_flLastActivityWarning;

    // So we don't lag compensate more than once.
    bool m_bIsFireBulletsRecursive;

    // Store all the ammo indices denied for this frame.
    CUtlVector<int> m_vAmmoDenied;
};

inline CZMPlayer* ToZMPlayer( CBaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return nullptr;

	return static_cast<CZMPlayer*>( pEntity );
}

inline CZMPlayer* ToZMPlayer( CBasePlayer* pPlayer )
{
	return static_cast<CZMPlayer*>( pPlayer );
}
