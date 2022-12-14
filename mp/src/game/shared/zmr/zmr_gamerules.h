#pragma once

#include "cbase.h"
#include "teamplay_gamerules.h"

#include "zmr_player_shared.h"
#include "zmr_shareddefs.h"

#ifndef CLIENT_DLL
#include "npcs/zmr_zombiebase_shared.h"
#include "zmr_entities.h"
#include "zmr_obj_manager.h"
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


class CZMViewVectors : public CViewVectors
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
        Vector vZMHullMin,
        Vector vZMHullMax,
        Vector vZMView,
        Vector vRatHullMin,
        Vector vRatHullMax,
        Vector vRatView ) :
            CViewVectors( 
                vView,
                vHullMin,
                vHullMax,
                vDuckHullMin,
                vDuckHullMax,
                vDuckView,
                vObsHullMin,
                vObsHullMax,
                vDeadViewHeight )
    {
        m_vZMHullMin = vZMHullMin;
        m_vZMHullMax = vZMHullMax;
        m_vZMView = vZMView;

        m_vRatHullMin = vRatHullMin;
        m_vRatHullMax = vRatHullMax;
        m_vRatView = vRatView;
    }

    Vector m_vZMHullMin;
    Vector m_vZMHullMax;
    Vector m_vZMView;

    Vector m_vRatHullMin;
    Vector m_vRatHullMax;
    Vector m_vRatView;
};


class CZMGameRulesProxy : public CGameRulesProxy // CGameRulesProxy CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CZMGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CZMRules : public CTeamplayRules //CHL2MPRules CTeamplayRules
{
public:
	DECLARE_CLASS( CZMRules, CTeamplayRules );

    // This makes datatables able to access our private vars.
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE(); 
#endif

    CZMRules();
    ~CZMRules();


    virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 ) OVERRIDE;

    virtual const CViewVectors* GetViewVectors() const OVERRIDE;
    const CZMViewVectors* GetZMViewVectors() const;

    static float GetMapRemainingTime();


    virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer ) OVERRIDE;

#ifndef CLIENT_DLL
    virtual void Precache() OVERRIDE;
    virtual void DeathNotice( CBasePlayer* pVictim, const CTakeDamageInfo& info ) OVERRIDE;
    virtual bool FShouldSwitchWeapon( CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon ) OVERRIDE;
    virtual int PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget ) OVERRIDE;
    virtual bool ClientCommand( CBaseEntity* pEdict, const CCommand& args ) OVERRIDE;


    void ExecuteMapConfigs();

    virtual void CreateStandardEntities() OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;

    void ClientSettingsChanged( CBasePlayer* pPlayer ) OVERRIDE;

    virtual const char* GetGameDescription() OVERRIDE;

    virtual void Think() OVERRIDE;

    virtual void PlayerSpawn( CBasePlayer* pPlayer ) OVERRIDE;
    virtual void PlayerThink( CBasePlayer* pPlayer ) OVERRIDE;

    virtual const char* GetChatFormat( bool bTeamOnly, CBasePlayer* pPlayer ) OVERRIDE;
    virtual const char* GetChatPrefix( bool bTeamOnly, CBasePlayer* pPlayer ) OVERRIDE { return ""; };
    virtual const char* GetChatLocation( bool bTeamOnly, CBasePlayer* pPlayer ) OVERRIDE { return nullptr; };

    virtual int WeaponShouldRespawn( CBaseCombatWeapon* pWeapon ) OVERRIDE;

    // Yes, always teamplay.
    virtual bool IsTeamplay( void ) OVERRIDE { return true; };
    virtual int GetAutoAimMode() OVERRIDE { return AUTOAIM_NONE; };

    virtual int ItemShouldRespawn( CItem* pItem ) OVERRIDE;
	virtual bool CanHaveAmmo( CBaseCombatCharacter* pPlayer, int iAmmoIndex ) OVERRIDE; // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter* pPlayer, const char* szName ) OVERRIDE;
    virtual bool CanHavePlayerItem( CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon ) OVERRIDE;

    virtual const char* SetDefaultPlayerTeam( CBasePlayer* pPlayer ) OVERRIDE;

    virtual CBaseEntity* GetPlayerSpawnSpot( CBasePlayer* pPlayer ) OVERRIDE;

    virtual bool IsSpawnPointValid( CBaseEntity* pSpawn, CBasePlayer* pPlayer ) OVERRIDE;
    
    virtual void ClientDisconnected( edict_t* pPlayer ) OVERRIDE;
    virtual void PlayerKilled( CBasePlayer* pPlayer, const CTakeDamageInfo& info ) OVERRIDE;

    virtual bool UseSuicidePenalty() OVERRIDE { return false; };
    virtual bool FlPlayerFallDeathDoesScreenFade( CBasePlayer* pPlayer ) OVERRIDE { return false; }; // Don't fade to eternal darkness when getting killed.

    virtual void InitDefaultAIRelationships() OVERRIDE;
    virtual bool FAllowNPCs() OVERRIDE;


    static void IncPopCount( ZombieClass_t );

    void OnClientFinishedPutInServer( CZMPlayer* pPlayer );

    bool CanInactivityPunish( CZMPlayer* pPlayer );
    void PunishInactivity( CZMPlayer* pPlayer );
    bool ReplaceZM( CZMPlayer* pPlayer );

    //bool RoundCleanupShouldIgnore( CBaseEntity* ) OVERRIDE;
    void EndRound( ZMRoundEndReason_t reason );
    void ResetWorld();
    void RestoreMap();


    bool ShouldLateSpawn( CZMPlayer* pPlayer );
    inline float GetRoundStartTime() const { return m_flRoundStartTime; };
    inline bool IsInRoundEnd() const { return m_bInRoundEnd; };


    static int GetNumAliveHumans();

    inline void SetZombiePop( int n ) { m_nZombiePop = n; };

    inline CZMEntLoadout* GetLoadoutEnt() const { return m_pLoadoutEnt; };
    inline void SetLoadoutEnt( CZMEntLoadout* pEnt ) { m_pLoadoutEnt = pEnt; }

    inline CZMEntObjectivesManager* GetObjManager() const { return m_pObjManager; };
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

protected:
#ifndef CLIENT_DLL
    virtual void GoToIntermission() OVERRIDE;
#endif

private:
    CNetworkVar( int, m_nZombiePop );
    CNetworkVar( int, m_nRounds ); // Number of "valid" rounds played. If the round is short, it will be skipped.
    CNetworkVar( int, m_nRealRounds ); // The real number of rounds played. Each round start is counted.

    

#ifndef CLIENT_DLL
    void RewardPoints( ZMRoundEndReason_t reason );

    CZMPlayer* ChooseZM();
    void BeginRound( CZMPlayer* pZM );


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
