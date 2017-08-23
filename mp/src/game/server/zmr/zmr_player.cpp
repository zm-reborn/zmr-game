#include "cbase.h"
#include "team.h"
//#include "baseplayer.h"

#include "player_pickup.h"

#include "ilagcompensationmanager.h"


#include "npcs/zmr_zombiebase.h"
#include "zmr_entities.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"
#include "weapons/zmr_carry.h"
#include "zmr_player.h"


static ConVar zm_sv_randomplayermodel( "zm_sv_randomplayermodel", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If player has an invalid model, use a random one. Temporary 'fix' for model choosing not working." );

ConVar zm_sv_antiafk( "zm_sv_antiafk", "90", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If the player is AFK for this many seconds, put them into spectator mode. 0 = disable" );


IMPLEMENT_SERVERCLASS_ST( CZMPlayer, DT_ZM_Player )
    SendPropDataTable( SENDINFO_DT( m_ZMLocal ), &REFERENCE_SEND_TABLE( DT_ZM_PlyLocal ), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()


BEGIN_DATADESC( CZMPlayer )
END_DATADESC()


LINK_ENTITY_TO_CLASS( player, CZMPlayer );
PRECACHE_REGISTER( player );


//LINK_ENTITY_TO_CLASS( info_player_deathmatch, CPointEntity ); // Is already defined in subs.cpp
LINK_ENTITY_TO_CLASS( info_player_survivor, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_zombiemaster, CPointEntity );


// ZMRTODO: Add support for loading models from file.
const char* g_ZMPlayerModels[] = 
{
    "models/humans/group02/male_01.mdl",
    "models/humans/group02/male_02.mdl",
    "models/humans/group02/female_01.mdl",
    "models/humans/group02/male_03.mdl",
    "models/humans/group02/female_02.mdl",
    "models/humans/group02/male_04.mdl",
    "models/humans/group02/female_03.mdl",
    "models/humans/group02/male_05.mdl",
    "models/humans/group02/female_04.mdl",
    "models/humans/group02/male_06.mdl",
    "models/humans/group02/female_06.mdl",
    "models/humans/group02/male_07.mdl",
    "models/humans/group02/female_07.mdl",
    "models/humans/group02/male_08.mdl",
    "models/humans/group02/male_09.mdl",
    "models/male_lawyer.mdl",
    "models/male_pi.mdl",
};


CZMPlayer::CZMPlayer()
{
    SetResources( 0 );

    m_flLastActivity = gpGlobals->curtime;
    m_flLastActivityWarning = 0.0f;

    m_flNextResourceInc = 0.0f;

    
    SetWeaponSlotFlags( 0 );
}

CZMPlayer::~CZMPlayer( void )
{

}

void CZMPlayer::Precache( void )
{
    // For now also precache the HL2DM stuff...
    BaseClass::Precache();
    //CBasePlayer::Precache();

    //PrecacheModel ( "sprites/glow01.vmt" );

#ifndef CLIENT_DLL
    // Can't be in gamerules object.
    UTIL_PrecacheOther( "npc_zombie" );
    UTIL_PrecacheOther( "npc_fastzombie" );
    UTIL_PrecacheOther( "npc_poisonzombie" );
    UTIL_PrecacheOther( "npc_dragzombie" );
    UTIL_PrecacheOther( "npc_burnzombie" );
#endif



#define DEF_PLAYER_MODEL    "models/male_pi.mdl"


    PrecacheModel( DEF_PLAYER_MODEL );

    for ( int i = 0 ; i < ARRAYSIZE( g_ZMPlayerModels ); i++ )
    {
        PrecacheModel( g_ZMPlayerModels[i] );
    }


    //PrecacheFootStepSounds();

    //PrecacheScriptSound( "NPC_MetroPolice.Die" );
    //PrecacheScriptSound( "NPC_CombineS.Die" );
    //PrecacheScriptSound( "NPC_Citizen.die" );
}

extern ConVar zm_sv_antiafk_punish;

ConVar zm_sv_flashlightdrainrate( "zm_sv_flashlightdrainrate", "0.15", 0, "How fast the flashlight battery drains per second. (out of 100)" );
ConVar zm_sv_flashlightrechargerate( "zm_sv_flashlightrechargerate", "0", 0, "How fast the flashlight battery recharges per second. (out of 100)" );

void CZMPlayer::PreThink( void )
{
    if ( m_afButtonLast != m_nButtons )
    {
        m_flLastActivity = gpGlobals->curtime;
    }


    if ( IsCloseToAFK() && ZMRules()->CanInactivityPunish( this ) )
    {
        if ( IsAFK() )
        {
            ZMRules()->PunishInactivity( this );
        }
        else if ( (m_flLastActivityWarning + 1.0f) < gpGlobals->curtime )
        {
            ClientPrint( this, HUD_PRINTCENTER, "You are about to get punished for being AFK!" );

            m_flLastActivityWarning = gpGlobals->curtime;
        }
    }


    if ( IsAlive() )
    {
        if ( FlashlightIsOn() )
        {
            SetFlashlightBattery( GetFlashlightBattery() - gpGlobals->frametime * zm_sv_flashlightdrainrate.GetFloat() );

            if ( GetFlashlightBattery() < 0.0f )
            {
                SetFlashlightBattery( 0.0f );
                FlashlightTurnOff();
            }
        }
        else
        {
            SetFlashlightBattery( GetFlashlightBattery() + gpGlobals->frametime * zm_sv_flashlightrechargerate.GetFloat() );
            
            if ( GetFlashlightBattery() > 100.0f )
            {
                SetFlashlightBattery( 100.0f );
            }
        }
    }


    BaseClass::PreThink();
}

void CZMPlayer::ChangeTeam( int iTeam )
{
    int oldteam = GetTeamNumber();

    // Change the team silently...
	CBasePlayer::ChangeTeam( iTeam, true, true );


    SetTeamSpecificProps();


    CZMRules* pRules = ZMRules();
    Assert( pRules );
    
    // If we changed teams in the middle of the round (late-joining, etc.) send objectives. 
    if ( iTeam != oldteam && !pRules->IsInRoundEnd() && !(oldteam == ZMTEAM_HUMAN && iTeam != ZMTEAM_SPECTATOR) )
    {
        if ( pRules->GetObjManager() )
        {
            pRules->GetObjManager()->ChangeDisplay( this );
        }
    }

    // See if we need to restart the round.
    // HACK: Don't check for it if our new team isn't spectator.
    // This makes sure we can test stuff while being the only survivor.
    if (!pRules->IsInRoundEnd()
    &&  oldteam > ZMTEAM_SPECTATOR
    &&  iTeam <= ZMTEAM_SPECTATOR)
    {
        CTeam* team = GetGlobalTeam( oldteam );
        if ( team && team->GetNumPlayers() <= 0 )
        {
            pRules->EndRound( ZMROUND_GAMEBEGIN );
        }
    }
}


void CZMPlayer::SetTeamSpecificProps()
{
    // To shut up the asserts...
    // These states aren't even used, except for going into observer mode.
    State_Transition( STATE_ACTIVE );


    RemoveFlag( FL_NOTARGET );


    switch ( GetTeamNumber() )
    {
    case ZMTEAM_ZM :
        m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
    case ZMTEAM_SPECTATOR :
        RemoveAllItems( true );

        // HACK: UpdatePlayerSound will make NPCs hear our "footsteps" even as a ZM when we're "on the ground".
        RemoveFlag( FL_ONGROUND );
        AddFlag( FL_NOTARGET );

        if ( IsZM() )
        {
            m_takedamage = DAMAGE_NO;

            SetMoveType( MOVETYPE_NOCLIP );
            SetCollisionGroup( COLLISION_GROUP_DEBRIS );
            AddSolidFlags( FSOLID_NOT_SOLID );
            AddEffects( EF_NODRAW );
        }
        else
        {
            // Apparently this sets the observer physics flag.
		    State_Transition( STATE_OBSERVER_MODE );
        }

        break;
    default : break;
    }
}


void CZMPlayer::PickDefaultSpawnTeam()
{
	if ( GetTeamNumber() == ZMTEAM_UNASSIGNED )
	{
        ChangeTeam( ZMTEAM_SPECTATOR );
    }
}

void CZMPlayer::GiveDefaultItems()
{
    RemoveAllItems( false );


    if ( !IsSuitEquipped() )
        EquipSuit( false ); // Don't play "effects" (hand showcase anim)
	

    GiveNamedItem( "weapon_zm_carry" );
    GiveNamedItem( "weapon_zm_fists" );


    CZMRules* pRules = ZMRules();

    if ( pRules )
    {
        CZMEntLoadout* pLoadout = pRules->GetLoadoutEnt();

        if ( pLoadout )
        {
            pLoadout->DistributeToPlayer( this );
        }
    }


	/*CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 45,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 6,	"Buckshot");
	CBasePlayer::GiveAmmo( 6,	"357" );

	if ( GetPlayerModelType() == PLAYER_SOUNDS_METROPOLICE || GetPlayerModelType() == PLAYER_SOUNDS_COMBINESOLDIER )
	{
		GiveNamedItem( "weapon_stunstick" );
	}
	else if ( GetPlayerModelType() == PLAYER_SOUNDS_CITIZEN )
	{
		GiveNamedItem( "weapon_crowbar" );
	}
	
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_frag" );
	GiveNamedItem( "weapon_physcannon" );

	const char *szDefaultWeaponName = engine->GetClientConVarValue( entindex(), "cl_defaultweapon" );

	CBaseCombatWeapon *pDefaultWeapon = Weapon_OwnsThisType( szDefaultWeaponName );

	if ( pDefaultWeapon )
	{
		Weapon_Switch( pDefaultWeapon );
	}
	else
	{
		Weapon_Switch( Weapon_OwnsThisType( "weapon_physcannon" ) );
	}*/
}

void CZMPlayer::EquipSuit( bool bPlayEffects )
{
    // Never play the effect.
    CBasePlayer::EquipSuit( false );

    m_HL2Local.m_bDisplayReticle = true;
}

void CZMPlayer::RemoveAllItems( bool removeSuit )
{
    BaseClass::RemoveAllItems( removeSuit );

    // ZMRTODO: See if this has any side-effects.
    // HACK: To stop ZM having a viewmodel. Just hide our viewmodel.
    if ( GetViewModel() )
    {
        GetViewModel()->AddEffects( EF_NODRAW );
    }
}

void CZMPlayer::FlashlightTurnOn()
{
    if ( IsHuman() && IsAlive() && GetFlashlightBattery() > 0.0f )
    {
        AddEffects( EF_DIMLIGHT );
        EmitSound( "HL2Player.FlashlightOn" );
    }
}

void CZMPlayer::SetPlayerModel( void )
{
    const char* pszFallback = zm_sv_randomplayermodel.GetBool() ?
        g_ZMPlayerModels[random->RandomInt( 0, ARRAYSIZE( g_ZMPlayerModels ) - 1 )] :
        DEF_PLAYER_MODEL;

    const char* szModelName = nullptr;
    const char* pszCurrentModelName = modelinfo->GetModelName( GetModel() );


    szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );


    int modelIndexCurrent = modelinfo->GetModelIndex( pszCurrentModelName );
    int modelIndex = modelinfo->GetModelIndex( szModelName );


    if ( modelIndex == -1 || !ValidatePlayerModel( szModelName ) )
    {
        if (modelIndexCurrent == -1
        ||  !ValidatePlayerModel( pszCurrentModelName )
        ||  zm_sv_randomplayermodel.GetBool() )
        {
            pszCurrentModelName = pszFallback;
            modelIndexCurrent = -1;
        }


        char szReturnString[512];
        Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
        engine->ClientCommand( edict(), szReturnString );

        modelIndex = -1;
    }

    if ( modelIndex == -1 )
    {
        szModelName = modelIndexCurrent != -1 ? pszCurrentModelName : pszFallback;
    }

    if ( modelIndexCurrent == -1 || modelIndex != modelIndexCurrent )
    {
        SetModel( szModelName );

        //SetupPlayerSoundsByModel( szModelName );
        m_iPlayerSoundType = PLAYER_SOUNDS_CITIZEN;

        m_flNextModelChangeTime = gpGlobals->curtime + 10.0f;
    }
}

bool CZMPlayer::ValidatePlayerModel( const char* szModelName )
{
    for ( int i = 0; i < ARRAYSIZE( g_ZMPlayerModels ); ++i )
    {
        if ( Q_stricmp( g_ZMPlayerModels[i], szModelName ) == 0 )
        {
            return true;
        }
    }

    return false;
}

void CZMPlayer::Spawn()
{
	PickDefaultSpawnTeam();
    
    // Must set player model before calling base class spawn...
    SetPlayerModel();
    // Collision group must be set before base class spawn.
    // Spawnpoint look-up depend on the fact that player's are solid which is done before setting player's collision group...
    SetCollisionGroup( COLLISION_GROUP_PLAYER );

	//BaseClass::Spawn();
    BaseClass::BaseClass::Spawn();


    SetTeamSpecificProps();


	if ( !IsObserver() )
	{
		pl.deadflag = false;


        if ( IsHuman() )
        {
		    RemoveSolidFlags( FSOLID_NOT_SOLID );
		    RemoveEffects( EF_NODRAW );


            GiveDefaultItems();
        }
	}


	SetNumAnimOverlays( 3 );
	ResetAnimation();

	m_nRenderFX = kRenderNormal;


    if ( IsHuman() )
	    AddFlag( FL_ONGROUND );
    else
        RemoveFlag( FL_ONGROUND );

	/*if ( HL2MPRules()->IsIntermission() )
	{
		AddFlag( FL_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
	}*/

    //m_bReady = false;


    // Set in BasePlayer
	//m_Local.m_bDucked = false;
	//SetPlayerUnderwater( false );


    // Reset activity. Makes sure we don't get insta-punished when spawning after spectating somebody, etc.
    m_flLastActivity = gpGlobals->curtime;

    SetFlashlightBattery( 100.0f );
}

void CZMPlayer::FireBullets( const FireBulletsInfo_t& info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );


	NoteWeaponFired();


    BaseClass::BaseClass::FireBullets( info );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

extern ConVar sv_maxunlag;

bool CZMPlayer::WantsLagCompensationOnNPC( const CZMBaseZombie* pZombie, const CUserCmd* pCmd, const CBitVec<MAX_EDICTS>* pEntityTransmitBits ) const
{
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pZombie->entindex() ) )
		return false;

#define NPC_MAXSPEED        100.0f
	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pZombie->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5f * NPC_MAXSPEED * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );
    
	float flCosAngle = DOT_45DEGREE;
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

void CZMPlayer::CommitSuicide( bool bExplode, bool bForce )
{
    if ( !IsHuman() ) return;

    
    BaseClass::CommitSuicide( bExplode, bForce );
}

void CZMPlayer::CommitSuicide( const Vector &vecForce, bool bExplode, bool bForce )
{
    if ( !IsHuman() ) return;

    
    BaseClass::CommitSuicide( vecForce, bExplode, bForce );
}

extern ConVar physcannon_maxmass;

void CZMPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
    if ( !IsHuman() || !IsAlive() ) return;


    PlayerAttemptPickup( this, pObject );
}

bool CZMPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
    // The visibility check was causing problems. (probably due to shitty collision boxes for the weapons...)
    // Had the same problem as the items.

    if ( !IsHuman() ) return false;

    // How would this even be possible?
    if ( pWeapon->GetOwner() ) return false;


    if ( !IsAllowedToPickupWeapons() )
        return false;

    
    if ( /*!Weapon_CanUse( pWeapon ) ||*/ !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
    {
        return false;
    }
    
    // Don't do any check at all since it seems to fail with rifle and shotgun. (again, probably due to shitty collisionboxes or something)
    // This is not needed anyway since we're close enough.
    /*
    QAngle ang;
    Vector end;
    IPhysicsObject* pPhys = pWeapon->VPhysicsGetObject();

    if ( pPhys ) pPhys->GetPosition( &end, &ang );
    else end = pWeapon->CollisionProp()->WorldSpaceCenter();

	trace_t tr;
    CTraceFilterSkipTwoEntities filter( this, pWeapon, COLLISION_GROUP_NONE );
    UTIL_TraceLine( EyePosition(), end, MASK_OPAQUE, &filter, &tr );

    if ( tr.fraction < 1.0f ) return false;
    */

    if ( Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) ) 
    {
        return false;
    }

    pWeapon->CheckRespawn();

    Weapon_Equip( pWeapon );

    return true;
}

CBaseEntity* CZMPlayer::EntSelectSpawnPoint( void )
{
    const char *pszFallback = "info_player_deathmatch";
    const char *pszSpawn = pszFallback;

    CBaseEntity* pTarget = nullptr;


    if ( IsZM() )
    {
        pszSpawn = "info_player_zombiemaster";

        pTarget = gEntList.FindEntityByClassname( nullptr, pszSpawn );
	    if ( pTarget == nullptr )
	    {
		    pszSpawn = pszFallback;
	    }
        else
        {
            return pTarget;
        }
    }

    CBaseEntity* pEnt;
    
    // Try to find a spawn that isn't taken.
    pEnt = nullptr;
    while ( (pEnt = gEntList.FindEntityByClassname( pEnt, pszSpawn )) != nullptr )
    {
        if ( g_pGameRules->IsSpawnPointValid( pEnt, this ) )
        {
            return pEnt;
        }
    }

    // Didn't work, now just find any spot.
    pEnt = gEntList.FindEntityByClassname( nullptr, pszFallback );
    if ( pEnt ) return pEnt;

    pEnt = gEntList.FindEntityByClassname( nullptr, "info_player_start" );
    if ( pEnt ) return pEnt;


    Warning( "Map has no spawnpoints! Returning world...\n" );

    return UTIL_EntityByIndex( 0 );
}

void CZMPlayer::DeselectAllZombies()
{
    CZMBaseZombie* pZombie;
    
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == this )
        {
            pZombie->SetSelector( 0 );
        }
    }
}

void CZMPlayer::PlayerUse( void )
{
    if ( !IsHuman() )
    {
        return;
    }

    BaseClass::PlayerUse();
}
