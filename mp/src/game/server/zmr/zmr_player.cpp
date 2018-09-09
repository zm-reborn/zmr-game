#include "cbase.h"
#include "team.h"
//#include "baseplayer.h"

#include "player_pickup.h"

#include "ilagcompensationmanager.h"
#include "predicted_viewmodel.h"
#include "filesystem.h"
#include "EntityFlame.h"

#include "npcs/zmr_zombiebase.h"
#include "zmr_player_ragdoll.h"
#include "zmr/zmr_viewmodel.h"
#include "zmr_entities.h"
#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_global_shared.h"
#include "weapons/zmr_carry.h"
#include "zmr_player.h"




#define VMHANDS_FALLBACKMODEL   "models/weapons/c_arms_citizen.mdl"
#define DEF_PLAYER_MODEL        "models/male_pi.mdl"



static ConVar zm_sv_randomplayermodel( "zm_sv_randomplayermodel", "1", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If player has an invalid model, use a random one. Temporary 'fix' for model choosing not working." );
static ConVar zm_sv_modelchangedelay( "zm_sv_modelchangedelay", "6", FCVAR_NOTIFY | FCVAR_ARCHIVE, "", true, 0.0f, false, 0.0f );

ConVar zm_sv_antiafk( "zm_sv_antiafk", "90", FCVAR_NOTIFY | FCVAR_ARCHIVE, "If the player is AFK for this many seconds, put them into spectator mode. 0 = disable" );


static ConVar zm_sv_npcheadpushoff( "zm_sv_npcheadpushoff", "200", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How much force is applied to the player when standing on an NPC." );

// This is for maps that use the OnBreak trick to name the ZM. This does not work in ZMR anymore.
static ConVar zm_sv_comp_zmtargetname( "zm_sv_comp_zmtargetname", "", 0, "Gives the ZM a targetname on spawn. This is for compatibility with old maps!!!" );


CUtlVector<CZMPlayerModelData*> CZMPlayer::m_PlayerModels;



void* SendProxy_SendNonLocalDataTable( const SendProp* pProp, const void* pStruct, const void* pVarData, CSendProxyRecipients* pRecipients, int objectID )
{
    pRecipients->SetAllRecipients();
    pRecipients->ClearRecipient( objectID - 1 );
    return (void*)pVarData;
}

BEGIN_SEND_TABLE_NOBASE( CZMPlayer, DT_ZMLocalPlayerExclusive )
    // Send high-resolution for local
    SendPropVector( SENDINFO( m_vecOrigin ), -1,  SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

    // Is there any point to sending pitch to local and not yaw?
    //SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CZMPlayer, DT_ZMNonLocalPlayerExclusive )
    // Send low-resolution for non-local
    SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

    SendPropFloat( SENDINFO_VECTORELEM( m_angEyeAngles, 0 ), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
    SendPropAngle( SENDINFO_VECTORELEM( m_angEyeAngles, 1 ), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CZMPlayer, DT_ZM_Player )
    SendPropDataTable( SENDINFO_DT( m_ZMLocal ), &REFERENCE_SEND_TABLE( DT_ZM_PlyLocal ), SendProxy_SendLocalDataTable ),

    SendPropInt( SENDINFO( m_iSpawnInterpCounter ), 4 ),
    SendPropEHandle( SENDINFO( m_hRagdoll ) ),

    // Data that only gets sent to the local player.
    SendPropDataTable( "zmlocaldata", 0, &REFERENCE_SEND_TABLE(DT_ZMLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
    // Data that gets sent to all other players
    SendPropDataTable( "zmnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_ZMNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),


    // Other players' water level is networked for animations.
    SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),
    SendPropExclude( "DT_LocalPlayerExclusive", "m_nWaterLevel" ),

    
    SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
    SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
    SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

    // playeranimstate and clientside animation takes care of these on the client
    SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

    SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
    SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
    SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),
END_SEND_TABLE()


BEGIN_DATADESC( CZMPlayer )
END_DATADESC()


LINK_ENTITY_TO_CLASS( player, CZMPlayer );
PRECACHE_REGISTER( player );


CZMPlayerAnimState* CreateZMPlayerAnimState( CZMPlayer* pPlayer );

CZMPlayer::CZMPlayer()
{
    m_angEyeAngles.Init();
    m_iSpawnInterpCounter = 0;
    m_bIsFireBulletsRecursive = false;
    m_iLastWeaponFireUsercmd = 0;
    m_bEnterObserver = false;
    m_flNextModelChangeTime = 0.0f;

    BaseClass::ChangeTeam( 0 );


    m_pPlayerAnimState = CreateZMPlayerAnimState( this );
    UseClientSideAnimation();

    SetResources( 0 );

    m_flLastActivity = gpGlobals->curtime;
    m_flLastActivityWarning = 0.0f;

    m_flNextResourceInc = 0.0f;

    m_flNextScufflerTime = 0.0f;
    
    SetWeaponSlotFlags( 0 );
}

CZMPlayer::~CZMPlayer( void )
{
    m_pPlayerAnimState->Release();
}

void CZMPlayer::AddDefaultPlayerModels( KeyValues* kv )
{
    KeyValues* pNewKey = new KeyValues( "" );

    KeyValues* pSub = pNewKey->FindKey( "_DefaultModel", true );
    pSub->SetString( "model", DEF_PLAYER_MODEL );

    kv->AddSubKey( pNewKey );
}

int CZMPlayer::PrecachePlayerModels( KeyValues* kv )
{
    if ( !kv ) return 0;


    int num = 0;

    KeyValues* pKey = kv->GetFirstSubKey();

    while ( pKey )
    {
        const char* name = pKey->GetName();
        const char* model = pKey->GetString( "model" );

        if ( *model )
        {
            if ( PrecacheModel( model ) != -1 ) // Valid model?
            {
                ++num;


                const char* hands = pKey->GetString( "handsmodel" );
                if ( *hands && PrecacheModel( model ) == -1 )
                {
                    pKey->SetString( "handsmodel", "" );

                    Warning( "Invalid hand model path '%s' ('%s')!\n", hands, name );
                }


                m_PlayerModels.AddToTail( new CZMPlayerModelData( pKey ) );
            }
            else
            {
                Warning( "Invalid model path '%s' ('%s')!\n", model, name );

                KeyValues* oldKey = pKey;

                pKey = pKey->GetNextKey();

                kv->RemoveSubKey( oldKey );
                oldKey->deleteThis();

                continue;
            }
        }
        else
        {
            Warning( "No model path for '%s'!\n", name );
        }

        pKey = pKey->GetNextKey();
    }

    return num;
}

void CZMPlayer::Precache()
{
    // Precache register makes sure we are already precached.
    if ( !IsPrecacheAllowed() ) return;


    BaseClass::Precache();

    // Needs to be precached for thirdperson flashlight beam.
    PrecacheModel ( "sprites/glow01.vmt" );

    PrecacheModel( VMHANDS_FALLBACKMODEL );


    m_PlayerModels.PurgeAndDeleteElements();

    KeyValues* kv = LoadPlayerModels();

    // Make sure at least one model gets precached.
    if ( !kv )
        AddDefaultPlayerModels( (kv = new KeyValues( "" )) );

    if ( !PrecachePlayerModels( kv ) )
    {
        Warning( "WARNING: No player models precached! Crash inbound!\n" );
    }

    if ( kv )
        kv->deleteThis();
}

void CZMPlayer::UpdateOnRemove()
{
    if ( m_hRagdoll.Get() )
    {
        UTIL_RemoveImmediate( m_hRagdoll.Get() );
        m_hRagdoll.Set( nullptr );
    }

    BaseClass::UpdateOnRemove();
}

void CZMPlayer::NoteWeaponFired()
{
    Assert( m_pCurrentCommand );
    if ( m_pCurrentCommand )
    {
        m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
    }
}

extern ConVar zm_sv_antiafk_punish;

ConVar zm_sv_flashlightdrainrate( "zm_sv_flashlightdrainrate", "0.4", 0, "How fast the flashlight battery drains per second. (out of 100)" );
ConVar zm_sv_flashlightrechargerate( "zm_sv_flashlightrechargerate", "0.1", 0, "How fast the flashlight battery recharges per second. (out of 100)" );

void CZMPlayer::PreThink( void )
{
    // Erase our denied ammo.
    m_vAmmoDenied.Purge();


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
        UpdateAccuracyRatio();


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

        // Force player off the NPCs head!
        if ( zm_sv_npcheadpushoff.GetFloat() != 0.0f && GetGroundEntity() && !GetGroundEntity()->IsStandable() && GetGroundEntity()->IsBaseZombie() )
        {
            // A hack to keep pushing the player in the same direction they're looking at.
            // People wanting to crowd surf will get angry otherwise.
            Vector vec;
            AngleVectors( QAngle( 0.0f, GetAbsAngles().y, 0.0f ), &vec );

            PushAway( GetAbsOrigin() + vec * -10.0f, zm_sv_npcheadpushoff.GetFloat() );
        }
    }


    // Fixes player not changing target after the first target has died.
    if ( GetObserverMode() > OBS_MODE_FREEZECAM )
        CheckObserverSettings();



    BaseClass::PreThink();

    SetMaxSpeed( ZM_WALK_SPEED );
    State_PreThink();

    // Reset bullet force accumulator, only lasts one frame
    m_vecTotalBulletForce = vec3_origin;
}

void CZMPlayer::PostThink()
{
    BaseClass::PostThink();
    
    if ( GetFlags() & FL_DUCKING )
    {
        SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
    }


    QAngle angles = GetLocalAngles();
    angles[PITCH] = 0;
    SetLocalAngles( angles );

    m_angEyeAngles = EyeAngles();
    m_pPlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );
}

void CZMPlayer::PlayerDeathThink()
{
    if( !IsObserver() )
    {
        BaseClass::PlayerDeathThink();
    }
}

void CZMPlayer::PushAway( const Vector& pos, float force )
{
    Vector fwd = GetAbsOrigin() - pos;
    fwd.z = 0.0f;

    VectorNormalize( fwd );


    Vector vel = force * fwd;
    vel.z = GetAbsVelocity().z; // Make sure we have our z-velocity the same in case we need to apply fall damage or something.

    SetAbsVelocity( vel );
}

bool CZMPlayer::ClientCommand( const CCommand &args )
{
    return CBasePlayer::ClientCommand( args );
}

void CZMPlayer::ChangeTeam( int iTeam )
{
    int oldteam = GetTeamNumber();

    // Change the team silently...
    CBasePlayer::ChangeTeam( iTeam, true, true );


    if ( oldteam != ZMTEAM_UNASSIGNED && ShouldSpawn() )
    {
        Spawn();
    }
    else
    {
        SetTeamSpecificProps();
    }


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

bool CZMPlayer::ShouldSpawn()
{
    int team = GetTeamNumber();

    if ( team == ZMTEAM_SPECTATOR )
        return false;


    return true;
}

ConVar zm_mp_forcecamera( "zm_mp_forcecamera", "0", FCVAR_NOTIFY | FCVAR_ARCHIVE, "Are spectators disallowed to spectate the ZM?" );

bool CZMPlayer::IsValidObserverTarget( CBaseEntity* pEnt )
{
    if ( !pEnt ) return false;

    // ZMRTODO: Allow zombie spectating, etc.
    if ( !pEnt->IsPlayer() )
    {
        if ( pEnt->IsBaseZombie() )
            return true;

        return false;
    }


    CZMPlayer* pPlayer = ToZMPlayer( pEnt );

    if ( !pPlayer )
        return false;

    // Don't spec observers or players who haven't picked a class yet
    if ( pPlayer->IsObserver() )
        return false;

    if( pPlayer == this )
        return false; // We can't observe ourselves.

    if ( pPlayer->m_lifeState == LIFE_RESPAWNABLE ) // target is dead, waiting for respawn
        return false;

    if ( pPlayer->m_lifeState == LIFE_DEAD || pPlayer->m_lifeState == LIFE_DYING )
    {
        if ( (pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
        {
            return false; // allow watching until 3 seconds after death to see death animation
        }
    }
        

    if ( zm_mp_forcecamera.GetBool() )
    {
        if ( pPlayer->IsZM() )
            return false;
    }

    return true;
}

CBaseEntity* CZMPlayer::FindNextObserverTarget( bool bReverse )
{
    // If we were spectating a zombie, find next zombie.
    CZMBaseZombie* pZombie;
    if (GetObserverTarget()
    &&  GetObserverMode() != OBS_MODE_ROAMING
    &&  (pZombie = dynamic_cast<CZMBaseZombie*>( GetObserverTarget() )) != nullptr )
    {
        CZMBaseZombie* pLoop;

        int origin = -1;
        int len = g_ZombieManager.GetNumZombies();
        
        g_ZombieManager.ForEachZombieRet( [ &origin, pZombie ]( int index, CZMBaseZombie* pLoopZombie )
        {
            if ( pZombie == pLoopZombie )
            {
                origin = index;
                return true;
            }

            return false;
        } );

        if ( origin != -1 )
        {
            int iDir = bReverse ? -1 : 1;
            int i = origin;
            while ( true )
            {
                i += iDir;

                if ( i >= len )
                    i = 0;
                else if ( i < 0 )
                    i = len - 1;

                if ( i == origin )
                    break;

                
                pLoop = g_ZombieManager.GetZombieByIndex( i );
                if ( pLoop && pLoop->IsAlive() )
                {
                    return pLoop;
                }
            }
        }
    }


    return BaseClass::FindNextObserverTarget( bReverse );
}

void CZMPlayer::SetTeamSpecificProps()
{
    // To shut up the asserts...
    // These states aren't even used, except for going into observer mode.
    State_Transition( ZMSTATE_ACTIVE );


    RemoveFlag( FL_NOTARGET );


    switch ( GetTeamNumber() )
    {
    case ZMTEAM_ZM :
    {
        m_Local.m_iHideHUD |= HIDEHUD_HEALTH;

        // This is for maps that use the OnBreak trick to name the ZM. This does not work in ZMR anymore.
        const char* targetname = zm_sv_comp_zmtargetname.GetString();
        if ( targetname && *targetname )
        {
            SetName( AllocPooledString( targetname ) );
        }

    }
    case ZMTEAM_SPECTATOR :
        RemoveAllItems( true );

        // HACK: UpdatePlayerSound will make NPCs hear our "footsteps" even as a ZM when we're "on the ground".
        RemoveFlag( FL_ONGROUND );
        SetGroundEntity( nullptr ); // ZM's ground entity doesn't get updated, must reset it here or the view can act up

        AddFlag( FL_NOTARGET );

        if ( IsZM() )
        {
            m_takedamage = DAMAGE_NO;

            SetMoveType( MOVETYPE_NOCLIP );
            SetCollisionGroup( COLLISION_GROUP_DEBRIS );
            AddSolidFlags( FSOLID_NOT_SOLID );

            // Client will simply not render the ZM.
            // This allows the ZM movement to be interpolated.
            //AddEffects( EF_NODRAW );

            // I'm not sure if the vphysics object does anything in ZM's case, but let's see what happens with this.
            VPhysicsDestroyObject();
        }
        else
        {
            // Apparently this sets the observer physics flag.
            State_Transition( ZMSTATE_OBSERVER_MODE );
        }


        if ( FlashlightIsOn() )
            FlashlightTurnOff();

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

// Override this function to stop AmmoDenied usermessage spam.
int CZMPlayer::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound )
{
    // Don't try to give the player invalid ammo indices.
    if ( nAmmoIndex < 0 )
        return 0;

    bool bCheckAutoSwitch = false;
    if ( !HasAnyAmmoOfType( nAmmoIndex ) )
    {
        bCheckAutoSwitch = true;
    }

    int nAdd = CBasePlayer::GiveAmmo( nCount, nAmmoIndex, bSuppressSound );

    // We've been denied the pickup, display a hud icon to show that.
    // Make sure we don't send this usermessage multiple times a frame.
    if ( nCount > 0 && nAdd == 0 && m_vAmmoDenied.Find( nAmmoIndex ) == m_vAmmoDenied.InvalidIndex() )
    {
        CSingleUserRecipientFilter user( this );
        user.MakeReliable();
        UserMessageBegin( user, "AmmoDenied" );
            WRITE_SHORT( nAmmoIndex );
        MessageEnd();


        m_vAmmoDenied.AddToTail( nAmmoIndex );
    }

    //
    // If I was dry on ammo for my best weapon and justed picked up ammo for it,
    // autoswitch to my best weapon now.
    //
    if ( bCheckAutoSwitch )
    {
        CBaseCombatWeapon* pWeapon = g_pGameRules->GetNextBestWeapon( this, GetActiveWeapon() );

        if ( pWeapon && pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
        {
            SwitchToNextBestWeapon( GetActiveWeapon() );
        }
    }

    return nAdd;
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


    // Change weapon to best we have.
    CBaseCombatWeapon* pWep = GetWeaponOfHighestSlot();
    
    if ( pWep )
    {
        Weapon_Switch( pWep );
    }
}

void CZMPlayer::ImpulseCommands()
{
    int iImpulse = GetImpulse();
    switch ( iImpulse )
    {
    case 201 : // Spraying
    {
        // Don't allow ZM to spray...
        if ( IsZM() ) return;

        break;
    }
    default : break;
    }

    BaseClass::ImpulseCommands();
}

void CZMPlayer::CheatImpulseCommands( int iImpulse )
{
    switch ( iImpulse )
    {
    case 101 :
    {
        if ( !sv_cheats->GetBool() ) return;

        
        GiveNamedItem( "weapon_zm_pistol" );
        GiveNamedItem( "weapon_zm_revolver" );
        GiveNamedItem( "weapon_zm_improvised" );
        GiveNamedItem( "weapon_zm_sledge" );
        GiveNamedItem( "weapon_zm_shotgun" );
        GiveNamedItem( "weapon_zm_rifle" );
        GiveNamedItem( "weapon_zm_mac10" );
        GiveNamedItem( "weapon_zm_molotov" );

        CBasePlayer::GiveAmmo( 80, "Pistol" );
        CBasePlayer::GiveAmmo( 36, "Revolver" );
        CBasePlayer::GiveAmmo( 24, "Buckshot" );
        CBasePlayer::GiveAmmo( 11, "357" );
        CBasePlayer::GiveAmmo( 60, "SMG1" );


        break;
    }
    // HL2 class has some stuff we don't want or need.
    default : CBasePlayer::CheatImpulseCommands( iImpulse ); break;
    }
}

void CZMPlayer::EquipSuit( bool bPlayEffects )
{
    // Never play the effect.
    CBasePlayer::EquipSuit( false );
}

void CZMPlayer::RemoveAllItems( bool removeSuit )
{
    BaseClass::RemoveAllItems( removeSuit );

    // ZMRTODO: See if this has any side-effects.
    // HACK: To stop ZM having a viewmodel. Just hide our viewmodel.
    if ( GetViewModel( VMINDEX_WEP ) )
    {
        GetViewModel()->AddEffects( EF_NODRAW );
    }
    if ( GetViewModel( VMINDEX_HANDS ) )
    {
        GetViewModel( VMINDEX_HANDS )->AddEffects( EF_NODRAW );
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

void CZMPlayer::FlashlightTurnOff()
{
    RemoveEffects( EF_DIMLIGHT );

    if ( IsHuman() && IsAlive() )
    {
        EmitSound( "HL2Player.FlashlightOff" );
    }
}

void CZMPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
    // CBasePlayer may still call this.
}

KeyValues* CZMPlayer::LoadPlayerModels()
{
    KeyValues* kv = new KeyValues( "PlayerModels" );
    if ( !kv->LoadFromFile( filesystem, "resource/zmoptions_playermodels.txt", "MOD" ) )
    {
        kv->deleteThis();
        return nullptr;
    }

    return kv;
}

bool CZMPlayer::SetPlayerModel()
{
    if ( !m_PlayerModels.Count() )
    {
        Assert( 0 );
        Warning( "No player models to select from! RIP\n" );
        return false;
    }


    bool changed = false;

    const char* pszFallback = zm_sv_randomplayermodel.GetBool() ?
        m_PlayerModels[random->RandomInt( 0, m_PlayerModels.Count() - 1 )]->GetModelName() :
        DEF_PLAYER_MODEL;

    const char* szModelName = nullptr;
    const char* pszCurrentModelName = modelinfo->GetModelName( GetModel() );


    szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );


    int modelIndexCurrent = modelinfo->GetModelIndex( pszCurrentModelName );
    int modelIndex = modelinfo->GetModelIndex( szModelName );


    if ( modelIndex == -1 || FindPlayerModel( szModelName ) == -1 )
    {
        if (modelIndexCurrent == -1
        ||  FindPlayerModel( pszCurrentModelName ) == -1
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
        changed = true;
    }


    if ( changed )
    {
        SetHandsData( GetPlayerModelData( szModelName ) );
    }

    m_flNextModelChangeTime = gpGlobals->curtime + zm_sv_modelchangedelay.GetFloat();

    return changed;
}

int CZMPlayer::FindPlayerModel( const char* model )
{
    int len = m_PlayerModels.Count();
    for ( int i = 0; i < len; ++i )
    {
        if ( Q_stricmp( m_PlayerModels[i]->GetModelName(), model ) == 0 )
        {
            return i;
        }
    }

    return -1;
}

void CZMPlayer::UpdatePlayerFOV()
{
    const char* szFov = engine->GetClientConVarValue( entindex(), "zm_cl_fov" );


    int newFov = szFov ? atoi( szFov ) : 0;

    newFov = clamp( newFov > 0 ? newFov : g_pGameRules->DefaultFOV(), ZM_MIN_FOV, MAX_FOV );


    if ( newFov != GetDefaultFOV() )
    {
        SetDefaultFOV( newFov );
    }
}

void CZMPlayer::Spawn()
{
    m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;


    PickDefaultSpawnTeam();
    
    // Must set player model before calling base class spawn...
    SetPlayerModel();
    // Collision group must be set before base class spawn.
    // Spawnpoint look-up depend on the fact that player's are solid which is done before setting player's collision group...
    SetCollisionGroup( COLLISION_GROUP_PLAYER );

    CBasePlayer::Spawn();


    SetTeamSpecificProps();


    if ( !IsObserver() )
    {
        pl.deadflag = false;


        if ( IsHuman() )
        {
            RemoveSolidFlags( FSOLID_NOT_SOLID );

            GiveDefaultItems();

            SetMaxSpeed( ZM_WALK_SPEED );
        }

        RemoveEffects( EF_NODRAW );
    }


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


    // Take 4x more damage when impacted by physics. (Same as HL2DM)
    m_impactEnergyScale = 4.0f;


    // Reset activity. Makes sure we don't get insta-punished when spawning after spectating somebody, etc.
    m_flLastActivity = gpGlobals->curtime;

    SetFlashlightBattery( 100.0f );


    // Don't display crosshair if the map is a background.
    if ( gpGlobals->eLoadType == MapLoad_Background )
    {
        ShowCrosshair( false );
    }


    DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
}

bool CZMPlayer::BecomeRagdollOnClient( const Vector& force )
{
    return true;
}

void CZMPlayer::CreateRagdollEntity()
{
    if ( m_hRagdoll.Get() )
    {
        UTIL_RemoveImmediate( m_hRagdoll.Get() );
        m_hRagdoll.Set( nullptr );
    }

    // If we already have a ragdoll, don't make another one.
    CZMRagdoll* pRagdoll = dynamic_cast<CZMRagdoll*>( m_hRagdoll.Get() );
    
    if ( !pRagdoll )
    {
        // create a new one
        pRagdoll = dynamic_cast<CZMRagdoll*>( CreateEntityByName( "zm_ragdoll" ) );
    }

    if ( pRagdoll )
    {
        pRagdoll->m_hPlayer = this;
        pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
        pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
        pRagdoll->m_nModelIndex = m_nModelIndex;
        pRagdoll->m_nForceBone = m_nForceBone;
        pRagdoll->m_vecForce = m_vecTotalBulletForce;
        pRagdoll->SetAbsOrigin( GetAbsOrigin() );

        // Copy the effect over.
        CEntityFlame* pFlame = dynamic_cast<CEntityFlame*>( GetEffectEntity() );
        if ( pFlame )
        {
            pFlame->AttachToEntity( pRagdoll );
            pRagdoll->SetEffectEntity( pFlame );

            SetEffectEntity( nullptr );
        }
    }

    // ragdolls will be removed on round restart automatically
    m_hRagdoll.Set( pRagdoll );
}

void CZMPlayer::FireBullets( const FireBulletsInfo_t& info )
{
    // Make sure we don't lag compensate twice.
    // This is called recursively from HandleShotImpactingGlass.
    if ( !m_bIsFireBulletsRecursive )
    {
        // Move ents back to history positions based on local player's lag
        lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );


        NoteWeaponFired();


        m_bIsFireBulletsRecursive = true;
        CBaseEntity::FireBullets( info );
        m_bIsFireBulletsRecursive = false;


        // Move ents back to their original positions.
        lagcompensation->FinishLagCompensation( this );
    }
    else
    {
        DevMsg( "Called FireBullets recursively!\n" );

        CBaseEntity::FireBullets( info );
    }
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

void CZMPlayer::Event_Killed( const CTakeDamageInfo &info )
{
    //update damage info with our accumulated physics force
    CTakeDamageInfo subinfo = info;
    subinfo.SetDamageForce( m_vecTotalBulletForce );


    // Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
    // because we still want to transmit to the clients in our PVS.
    CreateRagdollEntity();

    // We can't be on fire while dead!
    Extinguish();

    CBaseEntity* pEffect = GetEffectEntity();
    if ( pEffect != nullptr )
    {
        UTIL_Remove( pEffect );
        SetEffectEntity( nullptr );
    }


    BaseClass::Event_Killed( subinfo );

    if ( info.GetDamageType() & DMG_DISSOLVE )
    {
        if ( m_hRagdoll )
        {
            m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
        }
    }


    CBaseEntity *pAttacker = info.GetAttacker();

    if ( pAttacker )
    {
        int iScoreToAdd = 1;

        if ( pAttacker == this )
        {
            iScoreToAdd = -1;
        }

        GetGlobalTeam( pAttacker->GetTeamNumber() )->AddScore( iScoreToAdd );
    }


    FlashlightTurnOff();

    m_lifeState = LIFE_DEAD;

    RemoveEffects( EF_NODRAW );	// still draw player body
    StopZooming();
}

int CZMPlayer::OnTakeDamage( const CTakeDamageInfo& inputInfo )
{
    // Fix for molotov fire damaging other players.
    if ( !friendlyfire.GetBool() )
    {
        CBaseEntity* pAttacker = inputInfo.GetAttacker();

        if ( pAttacker && pAttacker->GetOwnerEntity() )
        {
            if ( pAttacker->GetOwnerEntity()->GetTeamNumber() >= ZMTEAM_SPECTATOR )
            {
                return 0;
            }
        }
    }

    m_vecTotalBulletForce += inputInfo.GetDamageForce();

    //gamestats->Event_PlayerDamage( this, inputInfo );

    return BaseClass::OnTakeDamage( inputInfo );
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

void CZMPlayer::DeathSound( const CTakeDamageInfo &info )
{
    if ( m_hRagdoll.Get() && m_hRagdoll.Get()->GetBaseAnimating()->IsDissolving() )
         return;


    const char* szStepSound = "NPC_Citizen.Die";

    const char *pModelName = STRING( GetModelName() );

    CSoundParameters params;
    if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
        return;

    Vector vecOrigin = GetAbsOrigin();
    
    CRecipientFilter filter;
    filter.AddRecipientsByPAS( vecOrigin );

    EmitSound_t ep;
    ep.m_nChannel = params.channel;
    ep.m_pSoundName = params.soundname;
    ep.m_flVolume = params.volume;
    ep.m_SoundLevel = params.soundlevel;
    ep.m_nFlags = 0;
    ep.m_nPitch = params.pitch;
    ep.m_pOrigin = &vecOrigin;

    EmitSound( filter, entindex(), ep );
}

extern ConVar physcannon_maxmass;

void CZMPlayer::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
    if ( !IsHuman() || !IsAlive() ) return;


    PlayerAttemptPickup( this, pObject );
}

KeyValues* CZMPlayer::GetPlayerModelData( const char* model )
{
    int len = m_PlayerModels.Count();
    for ( int i = 0; i < len; ++i )
    {
        if ( Q_strcmp( model, m_PlayerModels[i]->GetModelName() ) == 0 )
        {
            return m_PlayerModels[i]->GetModelData();
        }
    }

    return nullptr;
}

void CZMPlayer::SetHandsModel( const char* model )
{
    if ( !model || !(*model) ) return;

    CBaseViewModel* pVM = GetViewModel( VMINDEX_HANDS );
    if ( !pVM ) return;

    pVM->SetModel( model );
}

void CZMPlayer::SetHandsData( KeyValues* kv )
{
    if ( !kv ) return;

    CZMViewModel* pVM = static_cast<CZMViewModel*>( GetViewModel( VMINDEX_HANDS ) );
    if ( !pVM ) return;


    const char* szHands = kv->GetString( "handsmodel" );
    if ( *szHands )
    {
        int iHands = modelinfo->GetModelIndex( szHands );
        if ( iHands != -1 && pVM->GetModelIndex() != iHands )
        {
            pVM->SetModelIndex( iHands );
        }
    }

    int skin = kv->GetInt( "handsskin", 0 );
    if ( skin >= 0 && skin < MAXSTUDIOSKINS )
    {
        pVM->m_nSkin = skin;
    }

    Color clr = kv->GetColor( "handscolor" );
    if ( clr[0] != 0 || clr[1] != 0 || clr[2] != 0 )
    {
        pVM->SetModelColor2( clr[0] / 255.0f, clr[1] / 255.0f, clr[2] / 255.0f );
    }
}

void CZMPlayer::CreateViewModel( int index )
{
    // We should never create more than the first index.
    Assert( index == 0 );


    if ( GetViewModel( VMINDEX_WEP ) != nullptr )
    {
        if ( GetViewModel( VMINDEX_HANDS ) == nullptr )
        {
            Warning( "Weapon viewmodel exists but hands don't!!\n" );
        }

        return;
    }


    CBaseViewModel* vm = static_cast<CBaseViewModel*>( CreateEntityByName( "predicted_viewmodel" ) );
    
    if ( vm )
    {
        vm->SetAbsOrigin( GetAbsOrigin() );
        vm->SetOwner( this );
        vm->SetIndex( VMINDEX_WEP );
        DispatchSpawn( vm );
        vm->FollowEntity( this, false );
        m_hViewModel.Set( VMINDEX_WEP, vm );


        CZMViewModel* vmhands = static_cast<CZMViewModel*>( CreateEntityByName( "zm_viewmodel" ) );
    
        if ( vmhands )
        {
            vmhands->SetAbsOrigin( GetAbsOrigin() );
            vmhands->SetOwner( this );
            vmhands->SetIndex( VMINDEX_HANDS );
            DispatchSpawn( vmhands );
            vmhands->FollowEntity( vm, true ); // Sets moveparent.
            m_hViewModel.Set( VMINDEX_HANDS, vmhands );
        }
    }

    SetHandsModel( VMHANDS_FALLBACKMODEL );
    SetHandsData( GetPlayerModelData( STRING( GetModelName() ) ) );
}

bool CZMPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
    // The visibility check was causing problems. (probably due to shitty collision boxes for the weapons...)

    if ( !IsHuman() ) return false;

    // How would this even be possible?
    if ( pWeapon->GetOwner() ) return false;


    if ( !IsAllowedToPickupWeapons() )
        return false;

    
    if ( /*!Weapon_CanUse( pWeapon ) ||*/ !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
    {
        return false;
    }
    
    
    // Apparently, the reason why this wasn't working was because the center was in solid.
    trace_t tr;
    CTraceFilterSkipTwoEntities filter( this, pWeapon, COLLISION_GROUP_NONE );
    UTIL_TraceLine( pWeapon->WorldSpaceCenter(), EyePosition(), MASK_OPAQUE, &filter, &tr );

    if ( tr.fraction != 1.0f && !tr.startsolid ) return false;
    

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
    const char *pszSpawn = IsZM() ? "info_player_zombiemaster" : "info_player_survivor";


    CUtlVector<CBaseEntity*> vValidSpots;
    vValidSpots.Purge();



    bool bCheckEnabled = true;
    CBaseEntity* pEnt;
    
    // Find all valid spawn points.
    for ( int i = 0; i < 2; i++ )
    {
        // Loop through our wanted spawnpoint, if it fails, use fallback.
        for ( int j = 0; j < 2; j++ )
        {
            pEnt = nullptr;
            while ( (pEnt = gEntList.FindEntityByClassname( pEnt, pszSpawn )) != nullptr )
            {
                if ( (!bCheckEnabled || static_cast<CZMEntSpawnPoint*>( pEnt )->IsEnabled()) && g_pGameRules->IsSpawnPointValid( pEnt, this ) )
                {
                    if ( vValidSpots.Find( pEnt ) == -1 )
                        vValidSpots.AddToTail( pEnt );
                }
            }

            if ( vValidSpots.Count() )
                break;

            pszSpawn = pszFallback;
        }

        if ( vValidSpots.Count() )
            break;

        // If even that failed, don't check if the spawnpoint is enabled.
        bCheckEnabled = false;
    }

    if ( vValidSpots.Count() )
    {
        return vValidSpots[random->RandomInt( 0, vValidSpots.Count() - 1 )];
    }


    Warning( "Couldn't find a valid spawnpoint for %i, using first fallback...\n", entindex() );


    // Didn't work, now just find any spot.
    pEnt = gEntList.FindEntityByClassname( nullptr, pszSpawn );
    if ( pEnt ) return pEnt;

    pEnt = gEntList.FindEntityByClassname( nullptr, pszFallback );
    if ( pEnt ) return pEnt;

    pEnt = gEntList.FindEntityByClassname( nullptr, "info_player_start" );
    if ( pEnt ) return pEnt;


    Warning( "Map has no spawnpoints! Returning world...\n" );

    return GetContainingEntity( INDEXENT( 0 ) );
}

void CZMPlayer::DeselectAllZombies()
{
    g_ZombieManager.ForEachSelectedZombie( this, []( CZMBaseZombie* pZombie )
    {
        pZombie->SetSelector( 0 );
    } );
}

void CZMPlayer::PlayerUse( void )
{
    if ( !IsHuman() )
    {
        return;
    }

    BaseClass::PlayerUse();
}

CZMBaseWeapon* CZMPlayer::GetWeaponOfHighestSlot()
{
    CZMBaseWeapon* pWep = nullptr;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_LARGE )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_SIDEARM )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_MELEE )) != nullptr )
        return pWep;

    if ( (pWep = GetWeaponOfSlot( ZMWEAPONSLOT_EQUIPMENT )) != nullptr )
        return pWep;

    // Just default to fists.
    return ToZMBaseWeapon( Weapon_OwnsThisType( "weapon_zm_fists" ) );
}

CZMBaseWeapon* CZMPlayer::GetWeaponOfSlot( int slot )
{
    // You can search for multiple slots.
    if ( !(GetWeaponSlotFlags() & slot) )
        return nullptr;


    CZMBaseWeapon* pWep;

    for ( int i = 0; i < MAX_WEAPONS; i++ ) 
    {
        pWep = ToZMBaseWeapon( m_hMyWeapons[i].Get() );

        if ( pWep && pWep->GetSlotFlag() & slot )
        {
            return pWep;
        }
    }

    return nullptr;
}

CZMBaseWeapon* CZMPlayer::GetWeaponOfSlot( const char* szSlotName )
{
    if ( Q_stricmp( szSlotName, "sidearm" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_SIDEARM );
    }

    if ( Q_stricmp( szSlotName, "melee" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_MELEE );
    }

    if ( Q_stricmp( szSlotName, "large" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_LARGE );
    }

    if ( Q_stricmp( szSlotName, "equipment" ) == 0 )
    {
        return GetWeaponOfSlot( ZMWEAPONSLOT_EQUIPMENT );
    }

    return nullptr;
}

int CZMPlayer::ShouldTransmit( const CCheckTransmitInfo* pInfo )
{
    // "The most difficult thing in life is to know yourself." - Some guy
    // I searched for a fitting "know yourself" quote to joke about it but I shouldn't have put so much effort into this...
    if ( pInfo->m_pClientEnt == edict() )
    {
        return FL_EDICT_ALWAYS;
    }


    if ( IsEffectActive( EF_NODRAW ) )
        return FL_EDICT_DONTSEND;


    // Don't send observers at all.
    // The player doesn't get switched fast enough to warrant this.
    if ( IsObserver()
    /*&&  (gpGlobals->curtime - m_flDeathTime) > 0.5f
    &&  m_lifeState == LIFE_DEAD && (gpGlobals->curtime - m_flDeathAnimTime) > 0.5f*/ )
    {
        return FL_EDICT_DONTSEND;
    }



    CZMPlayer* pRecipientPlayer = static_cast<CZMPlayer*>( CBaseEntity::Instance( pInfo->m_pClientEnt ) );

    if ( !pRecipientPlayer )
        return FL_EDICT_DONTSEND;


    if ( IsZM() )
    {
        // Don't send us if the recipient is alive, they're not suppose to know about us.
        if ( !pRecipientPlayer->IsObserver() )
        {
            return FL_EDICT_DONTSEND;
        }
    }


    return CBaseEntity::ShouldTransmit( pInfo );
}

void CZMPlayer::State_Transition( ZMPlayerState_t newState )
{
    State_Leave();
    State_Enter( newState );
}


void CZMPlayer::State_Enter( ZMPlayerState_t newState )
{
    m_iPlayerState = newState;
    m_pCurStateInfo = State_LookupInfo( newState );

    // Initialize the new state.
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
        (this->*m_pCurStateInfo->pfnEnterState)();
}


void CZMPlayer::State_Leave()
{
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
    {
        (this->*m_pCurStateInfo->pfnLeaveState)();
    }
}


void CZMPlayer::State_PreThink()
{
    if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
    {
        (this->*m_pCurStateInfo->pfnPreThink)();
    }
}


CZMPlayerStateInfo* CZMPlayer::State_LookupInfo( ZMPlayerState_t state )
{
    // This table MUST match the 
    static CZMPlayerStateInfo playerStateInfos[] =
    {
        { ZMSTATE_ACTIVE,			"STATE_ACTIVE",			&CZMPlayer::State_Enter_ACTIVE, NULL, &CZMPlayer::State_PreThink_ACTIVE },
        { ZMSTATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CZMPlayer::State_Enter_OBSERVER_MODE,	NULL, &CZMPlayer::State_PreThink_OBSERVER_MODE }
    };

    for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
    {
        if ( playerStateInfos[i].m_iPlayerState == state )
            return &playerStateInfos[i];
    }

    return NULL;
}

bool CZMPlayer::StartObserverMode( int mode )
{
    //we only want to go into observer mode if the player asked to, not on a death timeout
    if ( m_bEnterObserver == true )
    {
        VPhysicsDestroyObject();
        return BaseClass::StartObserverMode( mode );
    }
    return false;
}

void CZMPlayer::StopObserverMode()
{
    m_bEnterObserver = false;
    BaseClass::StopObserverMode();
}

void CZMPlayer::State_Enter_OBSERVER_MODE()
{
    int observerMode = m_iObserverLastMode;
    if ( IsNetClient() )
    {
        const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
        if ( pIdealMode )
        {
            observerMode = atoi( pIdealMode );
            if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
            {
                observerMode = m_iObserverLastMode;
            }
        }
    }
    m_bEnterObserver = true;
    StartObserverMode( observerMode );
}

void CZMPlayer::State_PreThink_OBSERVER_MODE()
{
    // Make sure nobody has changed any of our state.
    //	Assert( GetMoveType() == MOVETYPE_FLY );
    Assert( m_takedamage == DAMAGE_NO );
    Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
    //	Assert( IsEffectActive( EF_NODRAW ) );

    // Must be dead.
    Assert( m_lifeState == LIFE_DEAD );
    Assert( pl.deadflag );
}


void CZMPlayer::State_Enter_ACTIVE()
{
    SetMoveType( MOVETYPE_WALK );
    
    // md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
    // is false, then they'll be spectating but blocking live players from moving.
    // RemoveSolidFlags( FSOLID_NOT_SOLID );
    
    m_Local.m_iHideHUD = 0;
}


void CZMPlayer::State_PreThink_ACTIVE()
{
    //we don't really need to do anything here. 
    //This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death


    UpdateLastKnownArea();
}

int CZMPlayer::GetZMCommandInterruptFlags() const
{
    const char* val = engine->GetClientConVarValue( entindex(), "zm_cl_zmunitcommandinterrupt" );

    if ( !val || !(*val) )
        return 0;

    return atoi( val );
}
