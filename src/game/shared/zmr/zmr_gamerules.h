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


    virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 ) override;

    virtual const CViewVectors* GetViewVectors() const override;
    const CZMViewVectors* GetZMViewVectors() const;

    static float GetMapRemainingTime();


    virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer ) override;

#ifndef CLIENT_DLL
    virtual void Precache() override;
    virtual void DeathNotice( CBasePlayer* pVictim, const CTakeDamageInfo& info ) override;
    virtual bool FShouldSwitchWeapon( CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon ) override;
    virtual int PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget ) override;
    virtual bool ClientCommand( CBaseEntity* pEdict, const CCommand& args ) override;


    void ExecuteMapConfigs();

    virtual void CreateStandardEntities() override;
    virtual void LevelInitPostEntity() override;

    void ClientSettingsChanged( CBasePlayer* pPlayer ) override;

    virtual const char* GetGameDescription() override;

    virtual void Think() override;

    virtual void PlayerSpawn( CBasePlayer* pPlayer ) override;
    virtual void PlayerThink( CBasePlayer* pPlayer ) override;

    virtual const char* GetChatFormat( bool bTeamOnly, CBasePlayer* pPlayer ) override;
    virtual const char* GetChatPrefix( bool bTeamOnly, CBasePlayer* pPlayer ) override { return ""; };
    virtual const char* GetChatLocation( bool bTeamOnly, CBasePlayer* pPlayer ) override { return nullptr; };

    virtual int WeaponShouldRespawn( CBaseCombatWeapon* pWeapon ) override;

    // Yes, always teamplay.
    virtual bool IsTeamplay( void ) override { return true; };
    virtual int GetAutoAimMode() override { return AUTOAIM_NONE; };

    virtual int ItemShouldRespawn( CItem* pItem ) override;
	virtual bool CanHaveAmmo( CBaseCombatCharacter* pPlayer, int iAmmoIndex ) override; // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter* pPlayer, const char* szName ) override;
    virtual bool CanHavePlayerItem( CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon ) override;

    virtual const char* SetDefaultPlayerTeam( CBasePlayer* pPlayer ) override;

    virtual CBaseEntity* GetPlayerSpawnSpot( CBasePlayer* pPlayer ) override;

    virtual bool IsSpawnPointValid( CBaseEntity* pSpawn, CBasePlayer* pPlayer ) override;
    
    virtual void ClientDisconnected( edict_t* pPlayer ) override;
    virtual void PlayerKilled( CBasePlayer* pPlayer, const CTakeDamageInfo& info ) override;

    virtual bool UseSuicidePenalty() override { return false; };
    virtual bool FlPlayerFallDeathDoesScreenFade( CBasePlayer* pPlayer ) override { return false; }; // Don't fade to eternal darkness when getting killed.

    virtual void InitDefaultAIRelationships() override;
    virtual bool FAllowNPCs() override;


    static int IncPopCount( ZombieClass_t );
    static void DecPopCount( int cost );

    void OnClientFinishedPutInServer( CZMPlayer* pPlayer );

    bool CanInactivityPunish( CZMPlayer* pPlayer );
    void PunishInactivity( CZMPlayer* pPlayer );
    bool ReplaceZM( CZMPlayer* pPlayer );

    //bool RoundCleanupShouldIgnore( CBaseEntity* ) override;
    void EndRound( ZMRoundEndReason_t reason );
    void ResetWorld();
    void RestoreMap();


    bool ShouldLateSpawn( CZMPlayer* pPlayer );
    inline float GetRoundStartTime() const { return m_flRoundStartTime; };
    inline bool IsInRoundEnd() const { return m_bInRoundEnd; };


    static int GetNumAliveHumans();

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
    virtual void GoToIntermission() override;
#endif

private:
    CNetworkVar( int, m_nZombiePop );
    CNetworkVar( int, m_nRounds ); // Number of "valid" rounds played. If the round is short, it will be skipped.
    CNetworkVar( int, m_nRealRounds ); // The real number of rounds played. Each round start is counted.

    

#ifndef CLIENT_DLL
    inline void SetZombiePop( int n ) { m_nZombiePop = n; };

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
