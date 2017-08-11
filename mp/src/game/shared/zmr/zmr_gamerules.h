#pragma once

#include "cbase.h"
#include "hl2mp/hl2mp_gamerules.h"

#include "zmr/zmr_player_shared.h"
#include "zmr_shareddefs.h"

#ifndef CLIENT_DLL
#include "zmr/npcs/zmr_zombiebase.h"
#include "zmr/zmr_entities.h"
#endif


#ifdef CLIENT_DLL
#define CZMRules C_ZMRules
#define CZMGameRulesProxy C_ZMGameRulesProxy
#endif

extern ConVar zm_sv_zombiemax;

extern ConVar zm_sv_popcost_shambler;
extern ConVar zm_sv_popcost_banshee;
extern ConVar zm_sv_popcost_hulk;
extern ConVar zm_sv_popcost_drifter;
extern ConVar zm_sv_popcost_immolator;

extern ConVar zm_sv_cost_shambler;
extern ConVar zm_sv_cost_banshee;
extern ConVar zm_sv_cost_hulk;
extern ConVar zm_sv_cost_drifter;
extern ConVar zm_sv_cost_immolator;


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

#ifndef CLIENT_DLL
    virtual void CreateStandardEntities() OVERRIDE;

    void ClientSettingsChanged( CBasePlayer* ) OVERRIDE;

    virtual const char* GetGameDescription( void ) OVERRIDE { return "Zombie Master Reborn"; } //

    virtual void Think() OVERRIDE;

    virtual void PlayerThink( CBasePlayer* ) OVERRIDE;

    
    virtual int WeaponShouldRespawn( CBaseCombatWeapon* ) OVERRIDE;

    // Yes, always teamplay.
    virtual bool IsTeamplay( void ) OVERRIDE { return true; };
    virtual int GetAutoAimMode() OVERRIDE { return AUTOAIM_NONE; };

    virtual int ItemShouldRespawn( CItem* ) OVERRIDE;

	virtual bool CanHaveAmmo( CBaseCombatCharacter*, int ) OVERRIDE; // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter*, const char* ) OVERRIDE;

    virtual const char* SetDefaultPlayerTeam( CBasePlayer* ) OVERRIDE;

    virtual CBaseEntity* GetPlayerSpawnSpot( CBasePlayer* ) OVERRIDE;

    virtual bool IsSpawnPointValid( CBaseEntity*, CBasePlayer* ) OVERRIDE;
    
    virtual void ClientDisconnected( edict_t* ) OVERRIDE;
    virtual void PlayerKilled( CBasePlayer*, const CTakeDamageInfo& ) OVERRIDE;

    virtual bool CanHavePlayerItem( CBasePlayer*, CBaseCombatWeapon* ) OVERRIDE;

    
    static void IncPopCount( ZombieClass_t );

    void OnClientFinishedPutInServer( CZMPlayer* );


    //bool RoundCleanupShouldIgnore( CBaseEntity* ) OVERRIDE;
    void EndRound( ZMRoundEndReason_t );
    void ResetWorld();
    void RestoreMap();


    inline float GetRoundStartTime() { return m_flRoundStartTime; };
    inline bool IsInRoundEnd() { return m_bInRoundEnd; };


    static int GetNumAliveHumans();

    inline void SetZombiePop( int n ) { m_nZombiePop = n; };

    inline CZMEntLoadout* GetLoadoutEnt() { return m_pLoadoutEnt; };
    inline void SetLoadoutEnt( CZMEntLoadout* pEnt ) { m_pLoadoutEnt = pEnt; }
#endif

    inline int GetZombiePop() { return m_nZombiePop; };


    static int GetServerParticipationFlags();

#ifndef CLIENT_DLL
    virtual void InitDefaultAIRelationships( void );
#endif

private:
    CNetworkVar( int, m_nZombiePop );


    

#ifndef CLIENT_DLL
    void RewardPointsKill();

    void RewardPoints( ZMRoundEndReason_t );
    void PrintRoundEndMessage( ZMRoundEndReason_t );

    CZMPlayer* ChooseZM();
    void BeginRound( CZMPlayer* );


    int m_flRoundRestartTime;
    int m_flRoundStartTime;
    bool m_bInRoundEnd;
    int m_nRounds;

    CZMEntLoadout* m_pLoadoutEnt;
#endif
};

inline CZMRules* ZMRules()
{
	return static_cast<CZMRules*>( g_pGameRules );
}