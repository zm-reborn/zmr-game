#include "cbase.h"
#include "team.h"
//#include "baseplayer.h"

#include "player_pickup.h"

#include "ilagcompensationmanager.h"


#include "npcs/zmr_zombiebase.h"
#include "zmr_entities.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"
#include "zmr_player.h"


IMPLEMENT_SERVERCLASS_ST( CZMPlayer, DT_ZM_Player )
    SendPropInt( SENDINFO( m_nResources ) ),
END_SEND_TABLE()


BEGIN_DATADESC( CZMPlayer )
END_DATADESC()


LINK_ENTITY_TO_CLASS( player, CZMPlayer );
PRECACHE_REGISTER( player );



//LINK_ENTITY_TO_CLASS( info_player_deathmatch, CPointEntity ); // Is already defined in subs.cpp
LINK_ENTITY_TO_CLASS( info_player_survivor, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_zombiemaster, CPointEntity );


CZMPlayer::CZMPlayer()
{
    SetResources( 0 );

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


#define DEF_PLAYER_MODEL    "models/male_pi.mdl"

    //PrecacheModel ( "sprites/glow01.vmt" );

#ifndef CLIENT_DLL
    // Can't be in gamerules object.
    UTIL_PrecacheOther( "npc_zombie" );
    UTIL_PrecacheOther( "npc_fastzombie" );
    UTIL_PrecacheOther( "npc_poisonzombie" );
    UTIL_PrecacheOther( "npc_dragzombie" );
    UTIL_PrecacheOther( "npc_burnzombie" );
#endif

    PrecacheModel( DEF_PLAYER_MODEL );

    //PrecacheFootStepSounds();

    //PrecacheScriptSound( "NPC_MetroPolice.Die" );
    //PrecacheScriptSound( "NPC_CombineS.Die" );
    //PrecacheScriptSound( "NPC_Citizen.die" );
}

void CZMPlayer::ChangeTeam( int iTeam )
{
    // Change the team silently...
	CBasePlayer::ChangeTeam( iTeam, true, true );


    SetTeamSpecificProps();
}


void CZMPlayer::SetTeamSpecificProps()
{
    // To shut up the asserts...
    // These states aren't even used, except for going into observer mode.
    State_Transition( STATE_ACTIVE );


    switch ( GetTeamNumber() )
    {
    case ZMTEAM_ZM :
        m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
    case ZMTEAM_SPECTATOR :
        RemoveAllItems( true );

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

void CZMPlayer::FlashlightTurnOn()
{
    if ( IsHuman() && IsAlive() )
    {
        AddEffects( EF_DIMLIGHT );
        EmitSound( "HL2Player.FlashlightOn" );
    }
}

void CZMPlayer::SetPlayerModel( void )
{
    const char *szModelName = nullptr;
    const char *pszCurrentModelName = modelinfo->GetModelName( GetModel() );

    szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

    if ( !ValidatePlayerModel( szModelName ) )
    {
        char szReturnString[512];

        if ( !ValidatePlayerModel( pszCurrentModelName ) )
        {
            pszCurrentModelName = DEF_PLAYER_MODEL;
        }

        Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
        engine->ClientCommand ( edict(), szReturnString );

        szModelName = pszCurrentModelName;
    }

    int modelIndex = modelinfo->GetModelIndex( szModelName );

    if ( modelIndex == -1 )
    {
        szModelName = DEF_PLAYER_MODEL;

        char szReturnString[512];

        Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
        engine->ClientCommand ( edict(), szReturnString );
    }

    SetModel( szModelName );

    //SetupPlayerSoundsByModel( szModelName );
    m_iPlayerSoundType = PLAYER_SOUNDS_CITIZEN;

    //m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

bool CZMPlayer::ValidatePlayerModel( const char* szModelName )
{
    // ZMRTODO: Implement all player models.
    return false;
}

void CZMPlayer::Spawn()
{
	PickDefaultSpawnTeam();
    
    // Must set player model before calling base class spawn...
    SetPlayerModel();
    

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


	AddFlag( FL_ONGROUND ); // set the player on the ground at the start of the round.

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

void CZMPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
    if ( !IsHuman() ) return;


    BaseClass::PickupObject( pObject, bLimitMassAndSize );
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
