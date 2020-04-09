#pragma once

#include "cbase.h"
#include "hl2mp/hl2mp_gamerules.h"

#include "zmr/zmr_player_shared.h"
#include "zmr_shareddefs.h"

#ifndef CLIENT_DLL
#include "npcs/zmr_zombiebase_shared.h"
#include "zmr/zmr_entities.h"
#include "zmr/zmr_obj_manager.h"
#endif


#ifdef CLIENT_DLL
#define CZMRules C_ZMRules
#define CZMGameRulesProxy C_ZMGameRulesProxy
#endif

extern ConVar zm_sv_zombiemax;


enum
{
    AFK_PUNISH_NOTHING = 0,
    AFK_PUNISH_SPECTATE,
    AFK_PUNISH_KICK
};


class CZMViewVectors : public HL2MPViewVectors
{
public:
    CZMViewVectors( 
        Vector vView,
        Vector vHullMin,
        Vector vHullMax,
        Vector vDuckHullMin,
        Vector vDuckHullMax,
        Vector vDuckView,
        Vector vObsHullMin,
        Vector vObsHullMax,
        Vector vDeadViewHeight,
        Vector vCrouchTraceMin,
        Vector vCrouchTraceMax,
        Vector vZMHullMin,
        Vector vZMHullMax,
        Vector vZMView ) :
            HL2MPViewVectors( 
                vView,
                vHullMin,
                vHullMax,
                vDuckHullMin,
                vDuckHullMax,
                vDuckView,
                vObsHullMin,
                vObsHullMax,
                vDeadViewHeight,
                vCrouchTraceMin,
                vCrouchTraceMax )
    {
        m_vZMHullMin = vZMHullMin;
        m_vZMHullMax = vZMHullMax;
        m_vZMView = vZMView;
    }

    Vector m_vZMHullMin;
    Vector m_vZMHullMax;
    Vector m_vZMView;
};


class CZMGameRulesProxy : public CGameRulesProxy // CGameRulesProxy CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CZMGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CZMRules : public CHL2MPRules //CHL2MPRules CTeamplayRules
{
public:
	DECLARE_CLASS( CZMRules, CHL2MPRules );

    // This makes datatables able to access our private vars.
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE(); 
#endif

    CZMRules();
    ~CZMRules( void );


    virtual void Precache( void ) OVERRIDE;

    virtual bool ShouldCollide( int, int ) OVERRIDE;

    virtual void DeathNotice( CBasePlayer*, const CTakeDamageInfo& ) OVERRIDE;


    virtual const CViewVectors* GetViewVectors() const;
    const CZMViewVectors* GetZMViewVectors() const;

#ifndef CLIENT_DLL
    void ExecuteMapConfigs();

    virtual void CreateStandardEntities() OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;

    void ClientSettingsChanged( CBasePlayer* ) OVERRIDE;

    virtual const char* GetGameDescription( void ) OVERRIDE { return ZMR_GAMEDESC; };

    virtual void Think() OVERRIDE;

    virtual void PlayerSpawn( CBasePlayer* pPlayer ) OVERRIDE;
    virtual void PlayerThink( CBasePlayer* ) OVERRIDE;

    virtual const char* GetChatFormat( bool bTeamOnly, CBasePlayer* ) OVERRIDE;
    virtual const char* GetChatPrefix( bool bTeamOnly, CBasePlayer* ) OVERRIDE { return ""; };
    virtual const char* GetChatLocation( bool bTeamOnly, CBasePlayer* ) OVERRIDE { return nullptr; };

    virtual int WeaponShouldRespawn( CBaseCombatWeapon* ) OVERRIDE;

    // Yes, always teamplay.
    virtual bool IsTeamplay( void ) OVERRIDE { return true; };
    virtual int GetAutoAimMode() OVERRIDE { return AUTOAIM_NONE; };

    virtual int ItemShouldRespawn( CItem* ) OVERRIDE;
	virtual bool CanHaveAmmo( CBaseCombatCharacter*, int ) OVERRIDE; // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter*, const char* ) OVERRIDE;
    virtual bool CanHavePlayerItem( CBasePlayer*, CBaseCombatWeapon* ) OVERRIDE;

    virtual const char* SetDefaultPlayerTeam( CBasePlayer* ) OVERRIDE;

    virtual CBaseEntity* GetPlayerSpawnSpot( CBasePlayer* ) OVERRIDE;

    virtual bool IsSpawnPointValid( CBaseEntity*, CBasePlayer* ) OVERRIDE;
    
    virtual void ClientDisconnected( edict_t* ) OVERRIDE;
    virtual void PlayerKilled( CBasePlayer*, const CTakeDamageInfo& ) OVERRIDE;

    virtual bool UseSuicidePenalty() OVERRIDE { return false; };
    virtual bool FlPlayerFallDeathDoesScreenFade( CBasePlayer* pPlayer ) OVERRIDE { return false; }; // Don't fade to eternal darkness when getting killed.

    virtual void InitDefaultAIRelationships() OVERRIDE;
    virtual bool FAllowNPCs() OVERRIDE;


    static void IncPopCount( ZombieClass_t );

    void OnClientFinishedPutInServer( CZMPlayer* );

    bool CanInactivityPunish( CZMPlayer* );
    void PunishInactivity( CZMPlayer* );
    bool ReplaceZM( CZMPlayer* );

    //bool RoundCleanupShouldIgnore( CBaseEntity* ) OVERRIDE;
    void EndRound( ZMRoundEndReason_t );
    void ResetWorld();
    void RestoreMap();


    bool ShouldLateSpawn( CZMPlayer* );
    inline float GetRoundStartTime() { return m_flRoundStartTime; };
    inline bool IsInRoundEnd() { return m_bInRoundEnd; };


    static int GetNumAliveHumans();

    inline void SetZombiePop( int n ) { m_nZombiePop = n; };

    inline CZMEntLoadout* GetLoadoutEnt() { return m_pLoadoutEnt; };
    inline void SetLoadoutEnt( CZMEntLoadout* pEnt ) { m_pLoadoutEnt = pEnt; }

    inline CZMEntObjectivesManager* GetObjManager() { return m_pObjManager; };
    inline void SetObjManager( CZMEntObjectivesManager* pEnt ) { m_pObjManager = pEnt; }

    CZMEntFogController* GetZMFogController() const { return m_pZMFog; }
#endif

    inline int GetZombiePop() const { return m_nZombiePop.Get(); }
    inline int GetRoundsPlayed( bool bReal = false ) const { return bReal ? m_nRealRounds.Get() : m_nRounds.Get(); }

    static int GetServerParticipationFlags();

#ifndef CLIENT_DLL
    static void RewardResources( int, bool bLimit = false );
    static void RewardScoreZM( int points );
#endif

private:
    CNetworkVar( int, m_nZombiePop );
    CNetworkVar( int, m_nRounds ); // Number of "valid" rounds played. If the round is short, it will be skipped.
    CNetworkVar( int, m_nRealRounds ); // The real number of rounds played. Each round start is counted.

    

#ifndef CLIENT_DLL
    void RewardPoints( ZMRoundEndReason_t );

    CZMPlayer* ChooseZM();
    void BeginRound( CZMPlayer* );


    int m_flRoundRestartTime;
    int m_flRoundStartTime;
    bool m_bInRoundEnd;
    int m_nRoundMaxHumansAlive;


    CZMEntLoadout* m_pLoadoutEnt;
    CZMEntObjectivesManager* m_pObjManager;
    CZMEntFogController* m_pZMFog;
#endif
};

inline CZMRules* ZMRules()
{
	return static_cast<CZMRules*>( g_pGameRules );
}
