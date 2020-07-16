#include "cbase.h"

#ifndef CLIENT_DLL
#include "basecombatcharacter.h"
#include "zmr_team.h"
#include "mapentities.h"
#include "gameinterface.h"
#include "eventqueue.h"
#include "viewport_panel_names.h"
#include "voice_gamemgr.h"
#endif

#ifndef CLIENT_DLL
#include "ammodef.h"

#include "zmr_voting.h"
#include "zmr_rejoindata.h"

#include "zmr_player.h"
#include "zmr_mapentities.h"
#include "zmr_resource_system.h"
#else
#include "c_zmr_player.h"
#endif

#include "zmr_gamerules.h"
#include "weapons/zmr_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
extern CAmmoDef* GetAmmoDef();
#endif

ConVar zm_sv_participation( "zm_sv_participation", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "0 = No limit, 1 = Don't allow only human, 2 = Don't allow only spec, 3 = Don't allow only spec/human" );

ConVar zm_sv_glow_item_enabled( "zm_sv_glow_item_enabled", "1", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "Is item (weapon/ammo) glow allowed?" );

#ifndef CLIENT_DLL
ConVar zm_sv_reward_points_zombiekill( "zm_sv_reward_points_zombiekill", "5", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many points ZM gets for killing a human with a zombie." );
ConVar zm_sv_reward_points_kill( "zm_sv_reward_points_kill", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many points ZM gets when a human dies." );

ConVar zm_sv_reward_zombiekill( "zm_sv_reward_zombiekill", "200", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many resources ZM gets for killing a human with a zombie." );
ConVar zm_sv_reward_kill( "zm_sv_reward_kill", "100", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many resources ZM gets when a human dies." );
#endif



#ifndef CLIENT_DLL
class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
    virtual bool CanPlayerHearPlayer( CBasePlayer* pListener, CBasePlayer* pTalker, bool& bProximity ) OVERRIDE
    {
        return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
    }
};

CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper* g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;
#endif


static CZMViewVectors g_ZMViewVectors(
    Vector( 0, 0, 64 ),         //VEC_VIEW (m_vView)
                              
    Vector(-16, -16, 0 ),       //VEC_HULL_MIN (m_vHullMin)
    Vector( 16,  16,  72 ),     //VEC_HULL_MAX (m_vHullMax)
                                                
    Vector(-16, -16, 0 ),       //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
    Vector( 16,  16,  36 ),     //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
    Vector( 0, 0, 35 ),         //VEC_DUCK_VIEW		(m_vDuckView)
                                                
    Vector(-10, -10, -10 ),     //VEC_OBS_HULL_MIN	(m_vObsHullMin)
    Vector( 10,  10,  10 ),     //VEC_OBS_HULL_MAX	(m_vObsHullMax)
                                                
    Vector( 0, 0, 14 ),         //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

    // To keep things like before, we need to keep the view offset the same.
    Vector( -16, -16, 48 ),    //VEC_ZM_HULL_MIN (vZMHullMin)
    Vector( 16, 16, 80 ),       //VEC_ZM_HULL_MAX (vZMHullMax)
    Vector( 0, 0, 64 )          //VEC_ZM_VIEW (vZMView)
);


REGISTER_GAMERULES_CLASS( CZMRules );

BEGIN_NETWORK_TABLE_NOBASE( CZMRules, DT_ZM_Rules )

    #ifdef CLIENT_DLL
        RecvPropInt( RECVINFO( m_nZombiePop ) ),
        RecvPropInt( RECVINFO( m_nRounds ), SPROP_UNSIGNED ),
        RecvPropInt( RECVINFO( m_nRealRounds ), SPROP_UNSIGNED ),
    #else
        SendPropInt( SENDINFO( m_nZombiePop ) ),
        SendPropInt( SENDINFO( m_nRounds ), -1, SPROP_UNSIGNED ),
        SendPropInt( SENDINFO( m_nRealRounds ), -1, SPROP_UNSIGNED ),
    #endif

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( zm_gamerules, CZMGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( ZMGameRulesProxy, DT_ZM_GameRulesProxy )



#ifdef CLIENT_DLL
    void RecvProxy_ZMRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
    {
        CZMRules *pRules = ZMRules();
        Assert( pRules );
        *pOut = pRules;
    }

    BEGIN_RECV_TABLE( CZMGameRulesProxy, DT_ZM_GameRulesProxy )
        RecvPropDataTable( "zm_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_ZM_Rules ), RecvProxy_ZMRules )
    END_RECV_TABLE()
#else
    void* SendProxy_ZMRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
    {
        CZMRules *pRules = ZMRules();
        Assert( pRules );
        return pRules;
    }

    BEGIN_SEND_TABLE( CZMGameRulesProxy, DT_ZM_GameRulesProxy )
        SendPropDataTable( "zm_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_ZM_Rules ), SendProxy_ZMRules )
    END_SEND_TABLE()
#endif



// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
const char* g_sTeamNames[] =
{
    "Unassigned",
    "Spectator",
    "Survivors",
    "Zombie Master",
};


ConVar zm_sv_zombiemax( "zm_sv_zombiemax", "50", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "" );


CZMRules::CZMRules()
{
#ifndef CLIENT_DLL
    m_flRoundRestartTime = 0.0f;
    m_flRoundStartTime = 0.0f;
    m_bInRoundEnd = false;
    m_nRounds = 0;
    m_nRealRounds = 0;
    m_nRoundMaxHumansAlive = 0;


    SetZombiePop( 0 );

    m_pLoadoutEnt = nullptr;
    m_pObjManager = nullptr;

    Assert( g_Teams.Size() <= 0 );

    // Create the team managers
    for ( int i = 0; i < ARRAYSIZE( g_sTeamNames ); i++ )
    {
        CZMTeam* pTeam = static_cast<CZMTeam*>( CreateEntityByName( "team_manager" ) );
        pTeam->Init( g_sTeamNames[i], i );

        g_Teams.AddToTail( pTeam );
    }



    ExecuteMapConfigs();
#endif
}

CZMRules::~CZMRules()
{
#ifndef CLIENT_DLL
    g_Teams.Purge();
#endif
}

#ifndef CLIENT_DLL
void CZMRules::ExecuteMapConfigs()
{
    const char* mapcfg = STRING( gpGlobals->mapname );
    if ( !mapcfg || !(*mapcfg) )
        return;
    

    const char* premapcfg = "mapconfig_pre";
    const char* postmapcfg = "mapconfig_post";


    Log( "Executing map config files: %s, %s and %s\n", premapcfg, mapcfg, postmapcfg );


    

    // Execute the pre- map config.
    engine->ServerCommand( UTIL_VarArgs( "exec %s\n", premapcfg ) );

    // Execute the map config.
    engine->ServerCommand( UTIL_VarArgs( "exec %s\n", mapcfg ) );

    // Execute the post- map config.
    engine->ServerCommand( UTIL_VarArgs( "exec %s\n", postmapcfg ) );
}

void CZMRules::CreateStandardEntities()
{
    DevMsg( "Creating standard entities...\n" );


    BaseClass::CreateStandardEntities();


    
    CBaseEntity* pEnt;

    // Create the entity that will send our data to the client.
    pEnt = CBaseEntity::Create( "zm_gamerules", vec3_origin, vec3_angle );
    Assert( pEnt );


    pEnt = CBaseEntity::Create( "zm_objectives_manager", vec3_origin, vec3_angle );
    Assert( pEnt );


    pEnt = CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );
    Assert( pEnt );

    if ( pEnt )
    {
        new CZMVoteRoundRestart();
    }
}

void CZMRules::LevelInitPostEntity()
{
    CZMBaseZombie::g_flLastZombieSound = 0.0f;

    BaseClass::LevelInitPostEntity();


    // Find / create ZM fog entity.
    CZMEntFogController* pEnt = static_cast<CZMEntFogController*>( gEntList.FindEntityByClassname( nullptr, "env_fog_controller_zm" ) );

    if ( !pEnt )
    {
        pEnt = static_cast<CZMEntFogController*>( CBaseEntity::Create( "env_fog_controller_zm", vec3_origin, vec3_angle ) );
        pEnt->SetGameCreated();
    }
    

    Assert( pEnt );
    m_pZMFog = pEnt;
}

void CZMRules::Precache()
{
}
#endif

float CZMRules::GetMapRemainingTime()
{
    // if timelimit is disabled, return 0
    if ( mp_timelimit.GetInt() <= 0 )
        return 0;

    // timelimit is in minutes
    float timeleft = ( mp_timelimit.GetInt() * 60.0f ) - gpGlobals->curtime;

    return timeleft;
}

ConVar zm_sv_playercollision( "zm_sv_playercollision", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Is player collision enabled?" );

bool CZMRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
    // Swap so that lowest is always first
    if ( collisionGroup0 > collisionGroup1 )
    {
        V_swap( collisionGroup0,collisionGroup1 );
    }

    if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
        collisionGroup1 == COLLISION_GROUP_WEAPON )
    {
        return false;
    }

    // Disable player collision.
    if (collisionGroup0 == COLLISION_GROUP_PLAYER && collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT
    &&  zm_sv_playercollision.GetInt() <= 1)
    {
        return false;
    }

    return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 );
}

bool CZMRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
    return true;
}

const CViewVectors* CZMRules::GetViewVectors() const
{
	return &g_ZMViewVectors;
}

const CZMViewVectors* CZMRules::GetZMViewVectors() const
{
	return &g_ZMViewVectors;
}

#ifndef CLIENT_DLL

bool CZMRules::FShouldSwitchWeapon( CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon )
{
    if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() )
    {
        // Player has an active item, so let's check cl_autowepswitch.
        const char* cl_autowepswitch = engine->GetClientConVarValue( pPlayer->entindex(), "cl_autowepswitch" );
        if ( cl_autowepswitch && Q_atoi( cl_autowepswitch ) <= 0 )
        {
            return false;
        }
    }

    return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
}

int CZMRules::PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget )
{
    if ( !pPlayer || !pTarget ) return GR_NOTTEAMMATE;

    return ( pPlayer->GetTeamNumber() == pTarget->GetTeamNumber() ) ? GR_TEAMMATE : GR_ENEMY;
}

bool CZMRules::ClientCommand( CBaseEntity* pEdict, const CCommand& args )
{
	if ( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	auto* pPlayer = ToZMPlayer( pEdict );

	if ( pPlayer && pPlayer->ClientCommand( args ) )
		return true;

    return false;
}

void CZMRules::DeathNotice( CBasePlayer* pVictim, const CTakeDamageInfo& info )
{
    // Work out what killed the player, and send a message to all clients about it
    const char* killer_weapon_name = "world";		// by default, the player is killed by the world
    int killer_ID = 0;

    // Find the killer & the scorer
    CBaseEntity* pInflictor = info.GetInflictor();
    CBaseEntity* pKiller = info.GetAttacker();
    CBasePlayer* pScorer = GetDeathScorer( pKiller, pInflictor );

    // Custom kill type?
    if ( info.GetDamageCustom() )
    {
        killer_weapon_name = GetDamageCustomString( info );
        if ( pScorer )
        {
            killer_ID = pScorer->GetUserID();
        }
    }
    else
    {
        // Is the killer a client?
        if ( pScorer )
        {
            killer_ID = pScorer->GetUserID();
            
            if ( pInflictor )
            {
                if ( pInflictor == pScorer )
                {
                    // If the inflictor is the killer,  then it must be their current weapon doing the damage
                    if ( pScorer->GetActiveWeapon() )
                    {
                        killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
                    }
                }
                else
                {
                    killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
                }
            }
        }
        else
        {
            killer_weapon_name = pInflictor->GetClassname();
        }

        // strip the NPC_* or weapon_* from the inflictor's classname
        if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
        {
            killer_weapon_name += 7;
        }
        else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
        {
            killer_weapon_name += 4;
        }
        else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
        {
            killer_weapon_name += 5;
        }
        else if ( strstr( killer_weapon_name, "physics" ) )
        {
            killer_weapon_name = "physics";
        }
    }

    IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
    if( event )
    {
        event->SetInt( "userid", pVictim->GetUserID() );
        event->SetInt( "attacker", killer_ID );
        event->SetString( "weapon", killer_weapon_name );
        event->SetInt( "priority", 7 );
        gameeventmanager->FireEvent( event );
    }
}


void CZMRules::ClientSettingsChanged( CBasePlayer* pPlayer )
{
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    if ( !pZMPlayer ) return;



    const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
    const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

    // If we're different.
    if ( Q_stricmp( szModelName, pCurrentModel ) != 0 )
    {
        // Too soon, set the cvar back to what it was.
        // Note: this will make this function be called again
        // but since our models will match it'll just skip this whole dealio.
        if ( pZMPlayer->GetNextModelChangeTime() >= gpGlobals->curtime )
        {
            engine->ClientCommand( pZMPlayer->edict(), UTIL_VarArgs( "cl_playermodel %s", pCurrentModel ) );

            ClientPrint(
                pZMPlayer,
                HUD_PRINTTALK,
                UTIL_VarArgs(
                    "Please wait %.1f more seconds before trying to switch player models.\n",
                    (pZMPlayer->GetNextModelChangeTime() - gpGlobals->curtime)
                ) );
            return;
        }


        pZMPlayer->SetPlayerModel();

        const char *pszNewModel = modelinfo->GetModelName( pZMPlayer->GetModel() );

        if ( pCurrentModel != pszNewModel )
        {
            ClientPrint( pZMPlayer, HUD_PRINTTALK, UTIL_VarArgs( "Your player model is: %s\n", pszNewModel ) );
        }
    }

    pZMPlayer->UpdatePlayerFOV();
    pZMPlayer->UpdatePlayerInterpNPC();
    pZMPlayer->UpdatePlayerZMVars();

    BaseClass::ClientSettingsChanged( pPlayer );
}

const char* CZMRules::GetGameDescription()
{
    return ZMR_GAMEDESC;
}

const char* CZMRules::GetChatFormat( bool bTeamOnly, CBasePlayer* pPlayer )
{
    if ( !pPlayer ) return nullptr;


    const char* pszFormat = nullptr;

    // team only
    if ( bTeamOnly )
    {
        if ( pPlayer->IsObserver() )
        {
            pszFormat = "ZM_Chat_Team_Spec";
        }
        else
        {
            pszFormat = "ZM_Chat_Team";
        }
    }
    // everyone
    else
    {
        pszFormat = "ZM_Chat_All";
    }

    return pszFormat;
}

static ConVar zm_sv_norestartround( "zm_sv_norestartround", "0" );


void CZMRules::Think()
{
    BaseClass::Think();

    
    if ( g_fGameOver )
    {
        if ( m_flIntermissionEndTime != 0.0f && m_flIntermissionEndTime < gpGlobals->curtime )
        {
            ChangeLevel();

            m_flIntermissionEndTime = 0.0f; // HACK
        }

        return;
    }

    if ( IsInRoundEnd() )
    {
        if ( m_flRoundRestartTime != 0.0f && m_flRoundRestartTime < gpGlobals->curtime && !zm_sv_norestartround.GetBool() )
        {
            ResetWorld();
        }
    }

    if ( GetMapRemainingTime() < 0 )
    {
        GoToIntermission();
        return;
    }
}

extern ConVar mp_chattime;

void CZMRules::GoToIntermission()
{
	if ( g_fGameOver )
		return;

	g_fGameOver = true;


	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();


	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		auto* pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );
	}
}


static ConVar zm_sv_weaponrespawn( "zm_sv_weaponrespawn", "0", FCVAR_NOTIFY );
static ConVar zm_sv_ammorespawn( "zm_sv_ammorespawn", "0", FCVAR_NOTIFY );

int CZMRules::WeaponShouldRespawn( CBaseCombatWeapon* pWeapon )
{
    if ( !zm_sv_weaponrespawn.GetBool() )
    {
        return GR_WEAPON_RESPAWN_NO;
    }

    return BaseClass::WeaponShouldRespawn( pWeapon );
}

int CZMRules::ItemShouldRespawn( CItem* pItem )
{
    if ( !zm_sv_ammorespawn.GetBool() )
    {
        return GR_ITEM_RESPAWN_NO;
    }

    return CMultiplayRules::ItemShouldRespawn( pItem );
}

bool CZMRules::CanHaveAmmo( CBaseCombatCharacter* pPlayer, int iAmmoIndex )
{
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    if ( !pZMPlayer || pZMPlayer->IsZM() ) return false;


    // Do we have a weapon to use the ammo with?
    // Using dynamic cast until hl2mp weapons are removed.
    CZMBaseWeapon* pWep = ToZMBaseWeapon( pZMPlayer->Weapon_GetWpnForAmmo( iAmmoIndex ) );
    if ( !pWep )
    {
        return false;
    }
    
    // Don't allow ammo pickup when reloading (singly reloading weps).
    CZMBaseWeapon* pActive = ToZMBaseWeapon( pZMPlayer->GetActiveWeapon() );
    if ( pActive && !pActive->CanPickupAmmo() )
        return false;

    // Do we have enough room?
    int room = pZMPlayer->GetAmmoRoom( iAmmoIndex ) - pZMPlayer->GetTotalAmmoAmount( iAmmoIndex );
    if ( room > 0 )
    {
        return true;
    }
    
    return false;
}

bool CZMRules::CanHaveAmmo( CBaseCombatCharacter* pPlayer, const char* szName )
{
    return CGameRules::CanHaveAmmo( pPlayer, GetAmmoDef()->Index( szName ) );
}

bool CZMRules::CanHavePlayerItem( CBasePlayer* pPlayer, CBaseCombatWeapon* pBaseWeapon )
{
    // Do we already have this slot's weapon?
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    CZMBaseWeapon* pWep = ToZMBaseWeapon( pBaseWeapon );

    if ( pZMPlayer && pWep )
    {
        if ( pWep->GetSlotFlag() != ZMWEAPONSLOT_NOLIMIT && pZMPlayer->GetWeaponSlotFlags() & pWep->GetSlotFlag() )
        {
            return false;
        }

    }

    return BaseClass::CanHavePlayerItem( pPlayer, pBaseWeapon );
}

void CZMRules::ClientDisconnected( edict_t* pClient )
{
    // Check if we should restart the round...

    CZMPlayer* pPlayer = ToZMPlayer( CBaseEntity::Instance( pClient ) );

    if ( pPlayer )
    {
        // Tell the rejoin system we're leaving.
        GetZMRejoinSystem()->OnPlayerLeave( pPlayer );



        CTeam* teamplayer = pPlayer->GetTeam();

        if ( !teamplayer )
        {
            Assert( 0 );
            Warning( "Player left the game but didn't have a team!" );
        }



        CTeam* teamhuman = GetGlobalTeam( ZMTEAM_HUMAN );
        CTeam* teamzm = GetGlobalTeam( ZMTEAM_ZM );

        if (teamplayer
        &&  (teamplayer == teamhuman || teamplayer == teamzm)
        &&  teamplayer->GetNumPlayers() <= 1 )
        {
            EndRound( ( teamplayer == teamzm ) ? ZMROUND_ZMSUBMIT : ZMROUND_HUMANDEAD );
        }


        // Remove the player from his team
        if ( teamplayer )
        {
            teamplayer->RemovePlayer( pPlayer );
        }
    }

    return BaseClass::ClientDisconnected( pClient );
}

void CZMRules::PlayerKilled( CBasePlayer* pVictim, const CTakeDamageInfo& info )
{
    CZMPlayer* pPlayer = ToZMPlayer( pVictim );


    if ( pPlayer->IsHuman() )
    {
        // ZMRTODO: Use lambdas or some shit to make this more streamlined.
        bool bIsZombieKill = info.GetAttacker() && info.GetAttacker()->IsBaseZombie();

        RewardResources( bIsZombieKill ?
            zm_sv_reward_zombiekill.GetInt() :
            zm_sv_reward_kill.GetInt() );

        RewardScoreZM( bIsZombieKill ?
            zm_sv_reward_points_zombiekill.GetInt() :
            zm_sv_reward_points_kill.GetInt() );
    }

    // Don't use team player count since we haven't been switched to spectator yet.
    if ( !IsInRoundEnd() && pPlayer && pPlayer->IsHuman() && GetNumAliveHumans() <= 1 )
    {
        EndRound( ZMROUND_HUMANDEAD );
    }

    BaseClass::PlayerKilled( pVictim, info );
}

ConVar zm_sv_joingrace( "zm_sv_joingrace", "60", FCVAR_NOTIFY );

void CZMRules::OnClientFinishedPutInServer( CZMPlayer* pPlayer )
{
    // Don't restart the round if we're loaded as a background.
    if ( gpGlobals->eLoadType == MapLoad_Background )
        return;


    if ( IsInRoundEnd() ) return;


    bool latespawn = ShouldLateSpawn( pPlayer );


    CTeam* teamhuman = GetGlobalTeam( ZMTEAM_HUMAN );
    CTeam* teamzm = GetGlobalTeam( ZMTEAM_ZM );

    int numhuman = teamhuman ? teamhuman->GetNumPlayers() : 0;
    int numzm = teamzm ? teamzm->GetNumPlayers() : 0;

    
    if ( numzm < 1 || (numhuman < 1 && !latespawn) )
    {
        EndRound( ZMROUND_GAMEBEGIN );
        return;
    }


    if ( latespawn )
    {
        pPlayer->ChangeTeam( ZMTEAM_HUMAN );

        // Increase pick priority here as well.
        pPlayer->SetPickPriority( pPlayer->GetPickPriority() + 1 );
    }
}


ConVar zm_sv_antiafk_replacezm( "zm_sv_antiafk_replacezm", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If the player afking is the only ZM, replace them." );
ConVar zm_sv_antiafk_punish( "zm_sv_antiafk_punish", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "0 = Don't do anything. 1 = Transfer to spectator team, 2 = Kick." );


bool CZMRules::CanInactivityPunish( CZMPlayer* pPlayer )
{
    // Ignore bots.
    // For testing purposes and ZM AI ;)
    if ( pPlayer->IsBot() )
        return false;


    if ( pPlayer->IsZM() && zm_sv_antiafk_replacezm.GetBool() )
    {
        return true;
    }


    switch ( zm_sv_antiafk_punish.GetInt() )
    {
    case AFK_PUNISH_SPECTATE :
        if ( pPlayer->IsObserver() ) return false;
        break;
    case AFK_PUNISH_NOTHING : return false;
    default : break;
    }

    return true;
}

void CZMRules::PunishInactivity( CZMPlayer* pPlayer )
{
    if (zm_sv_antiafk_replacezm.GetBool()
    &&  pPlayer->IsZM()
    &&  !IsInRoundEnd()
    &&  GetGlobalTeam( ZMTEAM_ZM )->GetNumPlayers() <= 1
    )
    {
        ReplaceZM( pPlayer );
    }


    switch ( zm_sv_antiafk_punish.GetInt() )
    {
    case AFK_PUNISH_SPECTATE :
    {
        if ( !pPlayer->IsObserver() )
        {
            pPlayer->ChangeTeam( ZMTEAM_SPECTATOR );
        }

        break;
    }
    case AFK_PUNISH_KICK :
        engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );
        break;
    default : break;
    }
}

bool CZMRules::ReplaceZM( CZMPlayer* pZM )
{
    // Find the best candidate to replace the ZM.
    CZMPlayer* pChoice = nullptr;

    CUtlVector<CZMPlayer*> vFirstChoice;
    CUtlVector<CZMPlayer*> vOther;
    vFirstChoice.Purge();
    vOther.Purge();


    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( pPlayer == pZM ) continue;

        if ( pPlayer->IsZM() ) continue;

        if ( pPlayer->IsHLTV() ) continue;


        switch( pPlayer->GetParticipation() )
        {
        case ZMPART_ALLOWZM :
        {
            if ( pPlayer->IsObserver() || !pPlayer->IsAlive() )
            {
                vFirstChoice.AddToTail( pPlayer );
            }

            vOther.AddToTail( pPlayer );
        }
        }
    }


    if ( vFirstChoice.Count() > 0 )
    {
        pChoice = vFirstChoice[random->RandomInt( 0, vFirstChoice.Count() - 1 )];
    }
    else if ( vOther.Count() > 0 )
    {
        pChoice = vOther[random->RandomInt( 0, vOther.Count() - 1 )];
    }

    if ( pChoice )
    {
        pChoice->ChangeTeam( ZMTEAM_ZM );
    }


    pZM->ChangeTeam( ZMTEAM_SPECTATOR );


    return pChoice != nullptr;
}

CBaseEntity* CZMRules::GetPlayerSpawnSpot( CBasePlayer* pPlayer )
{
    CBaseEntity* pSpawn = pPlayer->EntSelectSpawnPoint();
    Assert( pSpawn );

    pPlayer->SetLocalOrigin( pSpawn->GetAbsOrigin() + Vector( 0, 0, 1 ) );
    pPlayer->SetAbsVelocity( vec3_origin );
    pPlayer->SetLocalAngles( pSpawn->GetLocalAngles() );
    pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
    pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
    pPlayer->SnapEyeAngles( pSpawn->GetLocalAngles() );

    return pSpawn;
}

const char* CZMRules::SetDefaultPlayerTeam( CBasePlayer* pPlayer )
{
    return "";
}

void CZMRules::IncPopCount( ZombieClass_t zclass )
{
    CZMRules* pRules = ZMRules();
    Assert( pRules );

    if ( !pRules ) return;


    pRules->SetZombiePop( pRules->GetZombiePop() + CZMBaseZombie::GetPopCost( zclass ) );
}

void CZMRules::InitDefaultAIRelationships()
{
    int i, j;

    //  Allocate memory for default relationships
    CBaseCombatCharacter::AllocateDefaultRelationships();

    // First initialize table so we can report missing relationships
    for ( i = 0; i < NUM_AI_CLASSES; i++ )
    {
        for ( j = 0; j < NUM_AI_CLASSES; j++ )
        {
            // By default all relationships are neutral of priority zero
            CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
        }
    }
    

    CBaseCombatCharacter::SetDefaultRelationship( CLASS_ZOMBIE, CLASS_NONE,         D_NU, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_ZOMBIE, CLASS_PLAYER,       D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_ZOMBIE, CLASS_ZOMBIE,       D_NU, 0 );

    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_ZOMBIE,       D_HT, 0 );

    // Let's assume anything that doesn't have a class also hates survivors.
    // This is a quick workaround for zm_frozenfinale final boss not spotting survivors.
    // And for fucksake, in the future, use the ai_relationship-entity.
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_NONE, CLASS_PLAYER,         D_HT, 0 );
}

bool CZMRules::FAllowNPCs()
{
    // Make sure background maps don't generate node graph.
    if ( gpGlobals->eLoadType == MapLoad_Background )
        return false;


    return true;
}

ConVar zm_mp_roundlimit( "zm_mp_roundlimit", "0", FCVAR_NOTIFY, "How many rounds do we play before going into intermission. 0 = Disable" );
ConVar zm_sv_roundintermissiontime( "zm_sv_roundintermissiontime", "5", FCVAR_NOTIFY, "How many seconds of wait there is before another round begins." );
ConVar zm_sv_roundmintime( "zm_sv_roundmintime", "20", FCVAR_NOTIFY, "Minimum amount of time that is considered as a full round." );

void CZMRules::EndRound( ZMRoundEndReason_t reason )
{
    // Can't end the round when it has already ended!
    if ( IsInRoundEnd() ) return;


    IGameEvent* pEvent = nullptr;

    // Fire round end event for plugins
    pEvent = gameeventmanager->CreateEvent( "round_end", true );
    if ( pEvent )
    {
        pEvent->SetInt( "reason", reason );

        gameeventmanager->FireEvent( pEvent, true );
    }


    RewardPoints( reason );


    m_flRoundRestartTime = gpGlobals->curtime + zm_sv_roundintermissiontime.GetFloat();
    m_bInRoundEnd = true;


    // Tell clients we don't want to show objectives anymore.
    CZMEntObjectivesManager::ResetObjectives();


    ++m_nRealRounds;
    if ( (gpGlobals->curtime - m_flRoundStartTime) > zm_sv_roundmintime.GetFloat() && reason != ZMROUND_GAMEBEGIN )
    {
        ++m_nRounds;
    }

    // Check round limit.
    if ( zm_mp_roundlimit.GetInt() > 0 && m_nRounds >= zm_mp_roundlimit.GetInt() )
    {
        GoToIntermission();
    }


    // Will be used by clients in the future.
    pEvent = gameeventmanager->CreateEvent( "round_end_post", true );
    if ( pEvent )
    {
        pEvent->SetInt( "reason", reason );

        gameeventmanager->FireEvent( pEvent, false );
    }


    // Send round end outputs for mappers.
    CZMEntWin::OnRoundEnd( reason );
}

void CZMRules::RewardResources( int res, bool bLimit )
{
    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( pPlayer->IsZM() )
        {
            pPlayer->IncResources( res, bLimit );
        }
    }
}

void CZMRules::RewardScoreZM( int points )
{
    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( pPlayer && pPlayer->IsZM() )
        {
            pPlayer->IncrementFragCount( points );
        }
    }
}

void CZMRules::RewardPoints( ZMRoundEndReason_t reason )
{
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;


        switch ( reason )
        {
        case ZMROUND_HUMANDEAD :
        case ZMROUND_HUMANLOSE :
            if ( pPlayer->IsZM() )
            {
                pPlayer->IncrementFragCount( ZM_POINTS_ZM_WIN );
            }
            break;
        case ZMROUND_HUMANWIN :
            if ( pPlayer->IsHuman() && pPlayer->IsAlive() )
            {
                pPlayer->IncrementFragCount( ZM_POINTS_HUMAN_WIN );
            }
            break;
        default :
            break;
        }

    }
}

CZMPlayer* CZMRules::ChooseZM()
{
    int i;
    CZMPlayer* pPlayer;


    int nHighestPriority = -1;

    CUtlVector<CZMPlayer*> vBackupZMs;
    CUtlVector<CZMPlayer*> vZMs;
    CUtlVector<CZMPlayer*> vZMFirstChoices;

    vBackupZMs.Purge();
    vZMs.Purge();
    vZMFirstChoices.Purge();


    int partflags = GetServerParticipationFlags();

    for ( i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( pPlayer->IsHLTV() ) continue;


        switch ( pPlayer->GetParticipation() )
        {
        case ZMPART_ONLYHUMAN :
        {
            if ( partflags & ZMPARTFLAG_NOHUMANSONLY )
                vZMs.AddToHead( pPlayer );
            break;
        }
        case ZMPART_ONLYSPEC :
        {
            if ( partflags & ZMPARTFLAG_NOSPECONLY )
                vZMs.AddToHead( pPlayer );
            break;
        }
        default :
        case ZMPART_ALLOWZM :
            vZMs.AddToHead( pPlayer );

            if ( pPlayer->GetPickPriority() >= nHighestPriority )
            {
                // Remove the people who had lower priority.
                if ( pPlayer->GetPickPriority() > nHighestPriority )
                {
                    vZMFirstChoices.Purge();
                }


                vZMFirstChoices.AddToHead( pPlayer );
                nHighestPriority = pPlayer->GetPickPriority();
            }

            break;
        }
        
        vBackupZMs.AddToHead( pPlayer );
    }


    if ( vZMFirstChoices.Count() > 0 )
    {
        return vZMFirstChoices[random->RandomInt( 0, vZMFirstChoices.Count() - 1 )];
    }

    // Nothing to pick from...
    if ( !vZMs.Count() && !vBackupZMs.Count() ) return nullptr;


    if ( vZMs.Count() > 0 )
    {
        i = random->RandomInt( 0, vZMs.Count() - 1 );

        return vZMs[i];
    }

    i = random->RandomInt( 0, vBackupZMs.Count() - 1 );

    return vBackupZMs[i];
}

ConVar zm_sv_resource_init( "zm_sv_resource_init", "100", FCVAR_NOTIFY, "The initial resource amount the ZM has at the start." );

void CZMRules::BeginRound( CZMPlayer* pZM )
{
    CZMPlayer* pPlayer;


    int partflags = GetServerParticipationFlags();


    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( pPlayer->IsHLTV() ) continue;


        // Always increase the priority.
        pPlayer->SetPickPriority( pPlayer->GetPickPriority() + 1 );


        int team = ZMTEAM_HUMAN;

        if ( pPlayer != pZM )
        {
            switch ( pPlayer->GetParticipation() )
            {
            case ZMPART_ONLYSPEC :
                if ( !(partflags & ZMPARTFLAG_NOSPECONLY) ) // Can our participation be spec only?
                    team = ZMTEAM_SPECTATOR;
                break;
            default : break;
            }
        }
        else
        {
            team = ZMTEAM_ZM;

            // Reset priority once we're the ZM.
            pPlayer->SetPickPriority( 0 );
        }


        pPlayer->SetResources( zm_sv_resource_init.GetInt() );


        // Don't change team if we're already a spectator.
        if ( team != ZMTEAM_SPECTATOR || !pPlayer->IsObserver() )
        {
            pPlayer->ChangeTeam( team );
        }
    }


    m_nRoundMaxHumansAlive = GetNumAliveHumans();


    if ( pZM )
    {
        UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s is now the Zombie Master!\n", pZM->GetPlayerName() ) );
    }
}

void CZMRules::ResetWorld()
{
    Msg( "The round is restarting...\n" );

    IGameEvent* pEvent;
    
    // Send restart event for plugins.
    pEvent = gameeventmanager->CreateEvent( "round_restart_pre", true );
    if ( pEvent )
    {
        gameeventmanager->FireEvent( pEvent, true );
    }


    RestoreMap();



    // Reset ZM fog in case the values were updated.
    if ( GetZMFogController() )
    {
        GetZMFogController()->InitFog();
    }

    // Reset our loadout distribution.
    if ( GetLoadoutEnt() )
    {
        GetLoadoutEnt()->Reset();
    }

    g_ZMResourceSystem.OnRoundStart();

    // Choose ZM and begin the round!
    CZMPlayer* pZM = ChooseZM();

    BeginRound( pZM );



    if ( GetObjManager() ) // Has to be done after restoring, preferably after changing teams.
    {
        GetObjManager()->RoundStart();
    }


    m_flRoundStartTime = gpGlobals->curtime;

    m_flRoundRestartTime = 0.0f;
    m_bInRoundEnd = false;


    // This will be used for clients in the future.
    pEvent = gameeventmanager->CreateEvent( "round_restart_post", true );
    if ( pEvent )
    {
        gameeventmanager->FireEvent( pEvent, false );
    }
}

void CZMRules::RestoreMap()
{
    DevMsg( "Restoring map...\n" );

    g_ZMMapEntities.Restore();
}

void CZMRules::PlayerSpawn( CBasePlayer* pPlayer )
{
    auto* pZMPlayer = ToZMPlayer( pPlayer );
    if ( !pZMPlayer->IsHuman() )
        return;


    // Fire game_player_equip

    CBaseEntity* pEquip = nullptr;


    while ( (pEquip = gEntList.FindEntityByClassname( pEquip, "game_player_equip" )) != nullptr )
    {
        pEquip->Touch( pPlayer );
    }
}

void CZMRules::PlayerThink( CBasePlayer* pPlayer )
{
    BaseClass::PlayerThink( pPlayer );
}

bool CZMRules::IsSpawnPointValid( CBaseEntity* pSpot, CBasePlayer* pPlayer )
{
    if ( pPlayer->GetTeamNumber() == ZMTEAM_ZM )
        return true;

    // Use our own filter since if the player collisions are off, all players will bunch up together.
    class CZMTraceFilterSpawnPoint : public CTraceFilterSimple
    {
    public:
        CZMTraceFilterSpawnPoint( const IHandleEntity* passentity, int collisionGroup )
            : CTraceFilterSimple( passentity, collisionGroup )
        {
        }

        virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask )
        {
            CBaseEntity* pEntity = EntityFromEntityHandle( pHandleEntity );
            const CBaseEntity* pPass = EntityFromEntityHandle( GetPassEntity() );

            if ( pEntity == pPass )
                return false;

            if ( pEntity )
            {
                if ( pEntity->IsPlayer() )
                {
                    if ( pEntity->IsEffectActive( EF_NODRAW ) )
                        return false;


                    return true;
                }

                // HACK: Some map that shall remain unnamed uses func_brush to freeze players at round start.
                if ( pEntity->IsBSPModel() )
                {
                    return false;
                }
            }


            return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
        }
    };

    // Start off the ground since some maps have the spawns touching the floor.
    CZMTraceFilterSpawnPoint filter( pPlayer, COLLISION_GROUP_PLAYER );
    trace_t trace;
    UTIL_TraceHull( pSpot->GetAbsOrigin() + Vector( 0.0f, 0.0f, 1.0f ),
                    pSpot->GetAbsOrigin() + Vector( 0.0f, 0.0f, 2.0f ),
                    pPlayer->GetPlayerMins(),
                    pPlayer->GetPlayerMaxs(),
                    MASK_PLAYERSOLID,
                    &filter,
                    &trace );

    return trace.fraction == 1.0f;

    /*
    CBaseEntity *pEnt = NULL;

    for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (pEnt = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
    {
        // if ent is a client, don't spawn on 'em
        if ( pEnt->IsPlayer() && pEnt != pPlayer )
            return false;
    }

    return true;
    */
}


int CZMRules::GetNumAliveHumans()
{
    int num = 0;

    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( pPlayer && pPlayer->IsHuman() && pPlayer->IsAlive() )
        {
            ++num;
        }
    }
    
    return num;
}

bool CZMRules::ShouldLateSpawn( CZMPlayer* pPlayer )
{
    if ( pPlayer->GetTeamNumber() > ZMTEAM_SPECTATOR && pPlayer->IsAlive() )
        return false;

    if ( pPlayer->IsHLTV() )
        return false;


    Participation_t part = pPlayer->GetParticipation();

    if ( part == ZMPART_ONLYSPEC )
    {
        int flags = ZMRules()->GetServerParticipationFlags();

        
        if ( !(flags & ZMPARTFLAG_NOSPECONLY) ) // Server allows spec only?
            return false;
    }

    // Outside late spawning grace period?
    if ( gpGlobals->curtime < (ZMRules()->GetRoundStartTime() + zm_sv_joingrace.GetFloat()) )
    {
        return false;
    }

    
    // Check if we were spawned this round already.
    auto* pData = GetZMRejoinSystem()->FindPlayerData( pPlayer, "Late spawn" );

    return pData ? pData->Validate() : true;
}
#endif


int CZMRules::GetServerParticipationFlags()
{
    int flags = 0;

    switch ( zm_sv_participation.GetInt() )
    {
    case ZMSERVPART_NOHUMANSONLY : flags |= ZMPARTFLAG_NOHUMANSONLY; break;
    case ZMSERVPART_NOSPECONLY : flags |= ZMPARTFLAG_NOSPECONLY; break;
    case ZMSERVPART_NOHUMANSPECONLY : flags |= ZMPARTFLAG_NOHUMANSONLY | ZMPARTFLAG_NOSPECONLY; break;
    default : break;
    }

    return flags;
}
