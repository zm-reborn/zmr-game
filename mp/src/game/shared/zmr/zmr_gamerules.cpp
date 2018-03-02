#include "cbase.h"

#ifndef CLIENT_DLL
#include "basecombatcharacter.h"
#include "zmr/zmr_team.h"
#include "mapentities.h"
#include "gameinterface.h"
#include "teamplayroundbased_gamerules.h"
#include "eventqueue.h"
#endif

#ifndef CLIENT_DLL
#include "ammodef.h"

#include "zmr/zmr_voting.h"

#include "zmr/zmr_player.h"
#else
#include "zmr/c_zmr_player.h"
#endif

#include "zmr_gamerules.h"
#include "zmr/weapons/zmr_base.h"


#ifndef CLIENT_DLL
extern CAmmoDef* GetAmmoDef();
#endif

ConVar zm_sv_popcost_shambler( "zm_sv_popcost_shambler", "1", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_banshee( "zm_sv_popcost_banshee", "6", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_hulk( "zm_sv_popcost_hulk", "5", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_drifter( "zm_sv_popcost_drifter", "2", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_immolator( "zm_sv_popcost_immolator", "8", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );

ConVar zm_sv_cost_shambler( "zm_sv_cost_shambler", "10", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_banshee( "zm_sv_cost_banshee", "60", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_hulk( "zm_sv_cost_hulk", "70", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_drifter( "zm_sv_cost_drifter", "35", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_immolator( "zm_sv_cost_immolator", "100", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );

ConVar zm_sv_resource_max( "zm_sv_resource_max", "5000", FCVAR_NOTIFY | FCVAR_REPLICATED );

static ConVar zm_sv_participation( "zm_sv_participation", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "0 = No limit, 1 = Don't allow only human, 2 = Don't allow only spec, 3 = Don't allow only spec/human" );

ConVar zm_sv_glow_item_enabled( "zm_sv_glow_item_enabled", "1", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "Is item (weapon/ammo) glow allowed?" );

#ifndef CLIENT_DLL
static ConVar zm_sv_reward_zombiekill( "zm_sv_reward_zombiekill", "200", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many points ZM gets for killing a human with a zombie." );
static ConVar zm_sv_reward_kill( "zm_sv_reward_kill", "100", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many points ZM gets when a human dies." );
#endif


ConVar zm_sv_happyzombies( "zm_sv_happyzombies", "0", FCVAR_REPLICATED, "Happy, happy zombies :)" );



#ifndef CLIENT_DLL
static const char* g_PreserveEnts[] =
{
    "ai_network",
    "ai_hint",
    "hl2mp_gamerules", // Not used.
    "team_manager",
    "player_manager",
    "env_soundscape",
    "env_soundscape_proxy",
    "env_soundscape_triggerable",
    "env_sun",
    "env_wind",
    "env_fog_controller",
    //"func_brush", // Yup, this causes problems (eg. zebra)
    "func_wall",
    "func_buyzone",
    "func_illusionary",
    "infodecal",
    "info_projecteddecal",
    "info_node",
    "info_target",
    "info_node_hint",
    //"info_player_deathmatch",
    //"info_player_combine",
    //"info_player_rebel",
    "info_map_parameters",
    //"keyframe_rope", // Will fuck up maps that parent ropes to objects.
    //"move_rope",
    "info_ladder",
    "player",
    "point_viewcontrol",
    "scene_manager",
    "shadow_control",
    "sky_camera",
    "soundent",
    "trigger_soundscape",
    "viewmodel",
    "predicted_viewmodel",
    "worldspawn",
    "point_devshot_camera",

    // Our preserved ents...
    "zm_gamerules",
    "func_win",
    //"info_player_zombiemaster",
    //"info_player_survivor",
    "info_loadout",

    "zm_objectives_manager",
    "zm_viewmodel",

    "vote_controller",

    "", // END Marker
};
#endif


REGISTER_GAMERULES_CLASS( CZMRules );

BEGIN_NETWORK_TABLE_NOBASE( CZMRules, DT_ZM_Rules )

    #ifdef CLIENT_DLL
        RecvPropInt( RECVINFO( m_nZombiePop ) ),
        RecvPropInt( RECVINFO( m_nRounds ), SPROP_UNSIGNED ),
    #else
        SendPropInt( SENDINFO( m_nZombiePop ) ),
        SendPropInt( SENDINFO( m_nRounds ), -1, SPROP_UNSIGNED ),
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
    m_nRoundMaxHumansAlive = 0;


    SetZombiePop( 0 );

    m_pLoadoutEnt = nullptr;
    m_pObjManager = nullptr;

    // Remove old HL2MP teams.
    // This proobabbllyy isn't the best way of doing this...
    CTeam* pTeam;
    int i;

    for ( i = 0; i < g_Teams.Size(); i++ )
    {
        pTeam = g_Teams.Element( i );

        if ( pTeam )
        {
            DevMsg( "Removing old team: %s\n", pTeam->GetName() );

            UTIL_Remove( pTeam );
        }

        g_Teams.Remove( i );
        --i;
    }

    // Create the team managers
    for ( i = 0; i < ARRAYSIZE( g_sTeamNames ); i++ )
    {
        CZMTeam* pTeam = static_cast<CZMTeam*>( CreateEntityByName( "team_manager" ) );
        pTeam->Init( g_sTeamNames[i], i );

        g_Teams.AddToTail( pTeam );
    }
#endif
}

CZMRules::~CZMRules( void )
{
#ifndef CLIENT_DLL
    g_Teams.Purge();
#endif
}

#ifndef CLIENT_DLL
void CZMRules::CreateStandardEntities()
{
    DevMsg( "Creating standard entities...\n" );


    // NOTE: DO NOT CALL HL2MP RULES.
    CGameRules::CreateStandardEntities();


    
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
    BaseClass::LevelInitPostEntity();
}
#endif

void CZMRules::Precache( void )
{
    BaseClass::Precache();
}

static ConVar zm_sv_playercollision( "zm_sv_playercollision", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE | FCVAR_REPLICATED, "Is player collision enabled?" );

bool CZMRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
    // Disable player collision.
    if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT)
    &&  (collisionGroup1 == COLLISION_GROUP_PLAYER || collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT)
    &&  !zm_sv_playercollision.GetBool())
    {
        return false;
    }

    return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 );
}

void CZMRules::DeathNotice( CBasePlayer* pVictim, const CTakeDamageInfo& info )
{
#ifndef CLIENT_DLL
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
#endif
}

#ifndef CLIENT_DLL

void CZMRules::ClientSettingsChanged( CBasePlayer* pPlayer )
{
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    if ( !pZMPlayer ) return;



    const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
    const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

    //If we're different.
    if ( stricmp( szModelName, pCurrentModel ) )
    {
        //Too soon, set the cvar back to what it was.
        //Note: this will make this function be called again
        //but since our models will match it'll just skip this whole dealio.
        if ( pZMPlayer->GetNextModelChangeTime() >= gpGlobals->curtime )
        {
            char szReturnString[512];

            Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pCurrentModel );
            engine->ClientCommand ( pZMPlayer->edict(), szReturnString );

            Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %.1f more seconds before trying to switch player models.\n", (pZMPlayer->GetNextModelChangeTime() - gpGlobals->curtime) );
            ClientPrint( pZMPlayer, HUD_PRINTTALK, szReturnString );
            return;
        }


        pZMPlayer->SetPlayerModel();

        const char *pszNewModel = modelinfo->GetModelName( pZMPlayer->GetModel() );

        if ( pCurrentModel != pszNewModel )
        {
            char szReturnString[192];
            Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszNewModel );

            ClientPrint( pZMPlayer, HUD_PRINTTALK, szReturnString );
        }
    }

    pZMPlayer->UpdatePlayerFOV();

    // Don't use HL2DM rules, it'll change player's team.
    CTeamplayRules::ClientSettingsChanged( pPlayer );
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
    // Don't call HL2MP think since it has some stuff we wouldn't be able to control.
    CGameRules::Think();

    
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
    int room = GetAmmoDef()->MaxCarry( iAmmoIndex ) - pPlayer->GetAmmoCount( iAmmoIndex );
    if ( room > 0 && room > (pWep->GetDropAmmoAmount() * 0.5f) )
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
        CTeam* teamplayer = pPlayer->GetTeam();

        CTeam* teamhuman = GetGlobalTeam( ZMTEAM_HUMAN );
        CTeam* teamzm = GetGlobalTeam( ZMTEAM_ZM );

        if (teamplayer
        &&  (teamplayer == teamhuman || teamplayer == teamzm)
        &&  teamplayer->GetNumPlayers() <= 1 )
        {
            EndRound( ( teamplayer == teamzm ) ? ZMROUND_ZMSUBMIT : ZMROUND_HUMANDEAD );
        }
    }

    return BaseClass::ClientDisconnected( pClient );
}

void CZMRules::PlayerKilled( CBasePlayer* pVictim, const CTakeDamageInfo& info )
{
    CZMPlayer* pPlayer = ToZMPlayer( pVictim );


    if ( pPlayer->IsHuman() )
    {
        RewardResources( ( info.GetAttacker() && info.GetAttacker()->IsNPC() ) ?
            zm_sv_reward_zombiekill.GetInt() :
            zm_sv_reward_kill.GetInt() );
    }

    // Don't use team player count since we haven't been switched to spectator yet.
    if ( !IsInRoundEnd() && pPlayer && pPlayer->IsHuman() && GetNumAliveHumans() <= 1 )
    {
        EndRound( ZMROUND_HUMANDEAD );
    }

    // Don't call HL2MP...
    CTeamplayRules::PlayerKilled( pVictim, info );
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


    RewardPoints( reason );


    m_flRoundRestartTime = gpGlobals->curtime + zm_sv_roundintermissiontime.GetFloat();
    m_bInRoundEnd = true;


    // Tell clients we don't want to show objectives anymore.
    CZMEntObjectivesManager::ResetObjectives();


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
    IGameEvent* pEvent = gameeventmanager->CreateEvent( "round_end_post", true );
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

static ConVar zm_sv_resource_init( "zm_sv_resource_init", "100", FCVAR_NOTIFY, "The initial resource amount the ZM has at the start." );

void CZMRules::BeginRound( CZMPlayer* pZM )
{
    CZMPlayer* pPlayer;


    int partflags = GetServerParticipationFlags();


    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

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


    // Reset our loadout distribution.
    if ( GetLoadoutEnt() )
    {
        GetLoadoutEnt()->Reset();
    }

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

    // Recreate all the map entities from the map data (preserving their indices),
    // then remove everything else except the players.

    // Get rid of all entities except players.
    CBaseEntity *pCur = gEntList.FirstEnt();
    while ( pCur )
    {
        // remove entities that has to be restored on roundrestart (breakables etc)
        if ( !FindInList( g_PreserveEnts, pCur->GetClassname() ) )
        {
            UTIL_Remove( pCur );
        }

        pCur = gEntList.NextEnt( pCur );
    }

    // Really remove the entities so we can have access to their slots below.
    gEntList.CleanupDeleteList();

    // Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
    // could kill respawning CTs
    g_EventQueue.Clear();

    // Now reload the map entities.
    class CZMEntFilter : public IMapEntityFilter
    {
    public:
        virtual bool ShouldCreateEntity( const char *pClassname )
        {
            // Don't recreate the preserved entities.
            if ( !FindInList( g_PreserveEnts, pClassname ) )
            {
                return true;
            }
            else
            {
                // Increment our iterator since it's not going to call CreateNextEntity for this ent.
                if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
                    m_iIterator = g_MapEntityRefs.Next( m_iIterator );

                return false;
            }
        }


        virtual CBaseEntity* CreateNextEntity( const char *pClassname )
        {
            if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
            {
                // This shouldn't be possible. When we loaded the map, it should have used 
                // CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
                // with the same list of entities we're referring to here.
                Assert( false );
                return NULL;
            }
            else
            {
                CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
                m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

                if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
                {
                    // Doh! The entity was delete and its slot was reused.
                    // Just use any old edict slot. This case sucks because we lose the baseline.
                    return CreateEntityByName( pClassname );
                }
                else
                {
                    // Cool, the slot where this entity was is free again (most likely, the entity was 
                    // freed above). Now create an entity with this specific index.
                    return CreateEntityByName( pClassname, ref.m_iEdict );
                }
            }
        }

    public:
        int m_iIterator; // Iterator into g_MapEntityRefs.
    };

    CZMEntFilter filter;
    filter.m_iIterator = g_MapEntityRefs.Head();

    // DO NOT CALL SPAWN ON info_node ENTITIES!

    MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

static ConVar zm_sv_resource_rate( "zm_sv_resource_rate", "5", FCVAR_NOTIFY );
static ConVar zm_sv_resource_refill_min( "zm_sv_resource_refill_min", "35", FCVAR_NOTIFY );
static ConVar zm_sv_resource_refill_max( "zm_sv_resource_refill_max", "100", FCVAR_NOTIFY );
static ConVar zm_sv_resource_refill_roundstartcount( "zm_sv_resource_refill_roundstartcount", "1", FCVAR_NOTIFY, "Is refilling based on current human count or count at the start of the round." );

void CZMRules::PlayerThink( CBasePlayer* pPlayer )
{
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    if ( pZMPlayer && pZMPlayer->IsZM() && pZMPlayer->m_flNextResourceInc < gpGlobals->curtime )
    {
        int limit = zm_sv_resource_max.GetInt();

        if ( pZMPlayer->GetResources() < limit )
        {
            int num;
            if ( zm_sv_resource_refill_roundstartcount.GetBool() )
            {
                // Always choose the higher one. We never know if players late-spawn, etc.
                num = max( m_nRoundMaxHumansAlive, GetNumAliveHumans() );

                // Update max humans.
                m_nRoundMaxHumansAlive = num;
            }
            else
            {
                num = GetNumAliveHumans();
            }

            pZMPlayer->IncResources( (int)SimpleSplineRemapVal(
                    num,
                    1, gpGlobals->maxClients - 1,
                    zm_sv_resource_refill_min.GetFloat(),
                    zm_sv_resource_refill_max.GetFloat() ), true );
        }

        pZMPlayer->m_flNextResourceInc = gpGlobals->curtime + zm_sv_resource_rate.GetFloat();
    }


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

            if ( pEntity && pEntity->IsPlayer() )
            {
                if ( pEntity->IsEffectActive( EF_NODRAW ) )
                    return false;


                return true;
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


    Participation_t part = pPlayer->GetParticipation();

    if ( part == ZMPART_ONLYSPEC )
    {
        int flags = ZMRules()->GetServerParticipationFlags();

        
        if ( !(flags & ZMPARTFLAG_NOSPECONLY) ) // Server allows spec only?
            return false;
    }

    return gpGlobals->curtime < (ZMRules()->GetRoundStartTime() + zm_sv_joingrace.GetFloat());
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
