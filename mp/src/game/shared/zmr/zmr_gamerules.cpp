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
ConVar zm_sv_popcost_banshee( "zm_sv_popcost_banshee", "3", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_hulk( "zm_sv_popcost_hulk", "5", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_drifter( "zm_sv_popcost_drifter", "2", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_popcost_immolator( "zm_sv_popcost_immolator", "8", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );

ConVar zm_sv_cost_shambler( "zm_sv_cost_shambler", "10", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_banshee( "zm_sv_cost_banshee", "50", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_hulk( "zm_sv_cost_hulk", "75", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_drifter( "zm_sv_cost_drifter", "40", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );
ConVar zm_sv_cost_immolator( "zm_sv_cost_immolator", "100", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE );

static ConVar zm_sv_participation( "zm_sv_participation", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_ARCHIVE, "0 = No limit, 1 = Don't allow only human, 2 = Don't allow only spec, 3 = Don't allow only spec/human" );


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
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
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
    "info_player_zombiemaster",
    "info_player_survivor",
    "info_loadout",

	"", // END Marker
};
#endif


REGISTER_GAMERULES_CLASS( CZMRules );

BEGIN_NETWORK_TABLE_NOBASE( CZMRules, DT_ZM_Rules )

    #ifdef CLIENT_DLL
	    RecvPropInt( RECVINFO( m_nZombiePop ) ),
    #else
	    SendPropInt( SENDINFO( m_nZombiePop ) ),
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


    SetZombiePop( 0 );

    m_pLoadoutEnt = nullptr;

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
    //CreateCustomNetworkStringTables

    // NOTE: DO NOT CALL HL2MP RULES.
	CGameRules::CreateStandardEntities();


	// Create the entity that will send our data to the client.
#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "zm_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
}
#endif

void CZMRules::Precache( void )
{
    BaseClass::Precache();
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

    // Don't use HL2DM rules, it'll change player's team.
	CTeamplayRules::ClientSettingsChanged( pPlayer );
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
    CZMBaseWeapon* pWep = dynamic_cast<CZMBaseWeapon*>( pZMPlayer->Weapon_GetWpnForAmmo( iAmmoIndex ) );
    if ( !pWep )
    {
        return false;
    }

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

    CZMBaseWeapon* pWep = dynamic_cast<CZMBaseWeapon*>( pBaseWeapon );

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

    // Don't use team player count since we haven't been switched to spectator yet.
    if ( !IsInRoundEnd() && pPlayer && pPlayer->IsHuman() && GetNumAliveHumans() <= 1 )
    {
        EndRound( ZMROUND_HUMANDEAD );
    }

    // Don't call HL2MP...
    BaseClass::BaseClass::PlayerKilled( pVictim, info );
}

ConVar zm_sv_joingrace( "zm_sv_joingrace", "60", FCVAR_NOTIFY );

void CZMRules::OnClientFinishedPutInServer( CZMPlayer* pPlayer )
{
    if ( IsInRoundEnd() ) return;


    bool latespawn = false;

    // Should we late-spawn?
    if (pPlayer->GetTeamNumber() <= ZMTEAM_SPECTATOR && !pPlayer->IsAlive()
    &&  gpGlobals->curtime < (GetRoundStartTime() + zm_sv_joingrace.GetFloat()) )
    {
        latespawn = true;
    }

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
        pPlayer->Spawn();
    }
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

void CZMRules::InitDefaultAIRelationships( void )
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

ConVar zm_mp_roundlimit( "zm_mp_roundlimit", "10", FCVAR_NOTIFY, "How many round do we play before going into intermission." );
ConVar zm_sv_roundintermissiontime( "zm_sv_roundintermissiontime", "5", FCVAR_NOTIFY, "How many seconds of time there is before another round begins." );

void CZMRules::EndRound( ZMRoundEndReason_t reason )
{
    // Can't end the round when it has already ended!
    if ( IsInRoundEnd() ) return;


    RewardPoints( reason );

    PrintRoundEndMessage( reason );


    m_flRoundRestartTime = gpGlobals->curtime + zm_sv_roundintermissiontime.GetFloat();
    m_bInRoundEnd = true;


    // Check round limit.
    ++m_nRounds;

    if ( zm_mp_roundlimit.GetInt() && m_nRounds > zm_mp_roundlimit.GetInt() )
    {
        GoToIntermission();
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

void CZMRules::PrintRoundEndMessage( ZMRoundEndReason_t reason )
{
    switch ( reason )
    {
    case ZMROUND_HUMANDEAD :
        UTIL_ClientPrintAll( HUD_PRINTTALK, "The Zombie Master has slaying the living!\n" );
        break;
    case ZMROUND_HUMANLOSE :
        UTIL_ClientPrintAll( HUD_PRINTTALK, "The living have failed their objectives!\n" );
        break;
    case ZMROUND_HUMANWIN :
        UTIL_ClientPrintAll( HUD_PRINTTALK, "The living have prevailed!\n" );
        break;
    case ZMROUND_ZMSUBMIT :
        UTIL_ClientPrintAll( HUD_PRINTTALK, "The Zombie Master has submitted...\n" );
        break;
    case ZMROUND_GAMEBEGIN :
        UTIL_ClientPrintAll( HUD_PRINTTALK, "The game will begin...\n" );
        break;
    default :
        break;
    }
}

CZMPlayer* CZMRules::ChooseZM()
{
    int i;
    CZMPlayer* pPlayer;


    CUtlVector<CZMPlayer*> vBackupZMs;
    CUtlVector<CZMPlayer*> vZMs;

    vBackupZMs.Purge();
    vZMs.Purge();


    int partflags = GetServerParticipationFlags();

    for ( i = 0; i < gpGlobals->maxClients; i++ )
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
            break;
        }

        
        vBackupZMs.AddToHead( pPlayer );
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


    for ( int i = 0; i < gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;


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
        }


        pPlayer->SetResources( zm_sv_resource_init.GetInt() );


        // Don't change team if we're already a spectator.
        if ( team != ZMTEAM_SPECTATOR || !pPlayer->IsObserver() )
        {
            pPlayer->ChangeTeam( team );
            pPlayer->Spawn();
        }
    }


    if ( pZM )
    {
        UTIL_ClientPrintAll( HUD_PRINTTALK, "%s is now the Zombie Master!\n", pZM->GetPlayerName() );
    }
}

void CZMRules::ResetWorld()
{
    Msg( "The round is restarting...\n" );

    RestoreMap();

    // Reset our loadout distribution.
    if ( GetLoadoutEnt() )
    {
        GetLoadoutEnt()->Reset();
    }


    CZMPlayer* pZM = ChooseZM();

    BeginRound( pZM );


    m_flRoundStartTime = gpGlobals->curtime;

    m_flRoundRestartTime = 0.0f;
    m_bInRoundEnd = false;
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
static ConVar zm_sv_resource_max( "zm_sv_resource_max", "5000", FCVAR_NOTIFY );
static ConVar zm_sv_resource_refill_min( "zm_sv_resource_refill_min", "25", FCVAR_NOTIFY );
static ConVar zm_sv_resource_refill_max( "zm_sv_resource_refill_max", "75", FCVAR_NOTIFY );

void CZMRules::PlayerThink( CBasePlayer* pPlayer )
{
    CZMPlayer* pZMPlayer = ToZMPlayer( pPlayer );

    if ( pZMPlayer && pZMPlayer->IsZM() && pZMPlayer->m_flNextResourceInc < gpGlobals->curtime )
    {
        int limit = zm_sv_resource_max.GetInt();

        if ( pZMPlayer->GetResources() < limit )
        {
            int newres = pZMPlayer->GetResources() +
                (int)SimpleSplineRemapVal(
                    GetNumAliveHumans(),
                    1, gpGlobals->maxClients - 1,
                    zm_sv_resource_refill_min.GetFloat(),
                    zm_sv_resource_refill_max.GetFloat() );

            if ( newres > limit )
                newres = limit;

            pZMPlayer->SetResources( newres );
        }

        pZMPlayer->m_flNextResourceInc = gpGlobals->curtime + zm_sv_resource_rate.GetFloat();
    }


    BaseClass::PlayerThink( pPlayer );
}

bool CZMRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer )
{
	trace_t trace;
    UTIL_TraceHull( pSpot->GetAbsOrigin(),
                    pSpot->GetAbsOrigin() + Vector( 0.0f, 0.0f, 1.0f ),
                    pPlayer->GetPlayerMins(),
                    pPlayer->GetPlayerMaxs(),
                    MASK_PLAYERSOLID,
                    pPlayer,
                    COLLISION_GROUP_PLAYER,
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