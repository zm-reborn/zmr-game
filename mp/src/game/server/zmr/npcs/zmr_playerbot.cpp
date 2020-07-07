#include "cbase.h"
#include "gameinterface.h"
#include "in_buttons.h"


#include "npcr/npcr_schedule.h"
#include "npcr/npcr_senses.h"

#include "zmr_playerbot.h"

#include "npcs/sched/zmr_bot_main.h"



ConVar zm_sv_debug_bot_lookat( "zm_sv_debug_bot_lookat", "0" );


class CZMPlayerSenses : public NPCR::CBaseSenses
{
public:
    CZMPlayerSenses( CZMPlayerBot* pNPC ) : NPCR::CBaseSenses( pNPC )
    {
    }

    virtual void FindNewEntities( CUtlVector<CBaseEntity*>& vListEntities ) OVERRIDE
    {
        CBaseEntity* pEnt = nullptr;
        while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "npc_*" )) != nullptr )
        {
            if ( pEnt->IsBaseZombie() && pEnt->IsAlive() )
                vListEntities.AddToTail( pEnt );
        }
    }

    virtual int GetSoundMask() const OVERRIDE { return SOUND_COMBAT | SOUND_DANGER; }
};


extern ConVar bot_mimic;
extern ConVar bot_mimic_pitch_offset;
extern ConVar bot_mimic_yaw_offset;
extern ConVar bot_mimic_target;
extern ConVar bot_attack;


LINK_ENTITY_TO_CLASS( npcr_player_zm, CZMPlayerBot );
PRECACHE_REGISTER( npcr_player_zm );


CZMPlayerBot::CZMPlayerBot()
{
    m_hFollowTarget.Set( nullptr );
}

CZMPlayerBot::~CZMPlayerBot()
{
}

NPCR::CScheduleInterface* CZMPlayerBot::CreateScheduleInterface()
{
    return new NPCR::CScheduleInterface( this, new BotMainSchedule );
}

NPCR::CBaseSenses* CZMPlayerBot::CreateSenses()
{
    return new CZMPlayerSenses( this );
}

bool CZMPlayerBot::ShouldUpdate() const
{
    if ( m_lifeState != LIFE_ALIVE )
        return false;

    return CBaseNPC::ShouldUpdate();
}

void CZMPlayerBot::Spawn()
{
    BaseClass::Spawn();

    NPCR::CEventDispatcher::OnSpawn();
}

CZMPlayer* CZMPlayerBot::CreateZMBot( const char* playername )
{
    char name[128];
    Q_strncpy( name, ( playername && (*playername) ) ? playername : "L Ron Hubbard", sizeof( name ) );


    CZMPlayer* pPlayer = NPCR::CPlayer<CZMPlayer>::CreateBot<CZMPlayerBot>( name );

    return pPlayer;
}

CBasePlayer* CZMPlayerBot::BotPutInServer( edict_t* pEdict, const char* playername )
{
    CZMPlayer* pPlayer = CreatePlayer( "npcr_player_zm", pEdict );

    if ( pPlayer )
    {
        pPlayer->SetPlayerName( playername );
        pPlayer->SetPickPriority( 0 );

        pPlayer->ClearFlags();
        pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
    }
    
    return pPlayer;
}

bool CZMPlayerBot::OverrideUserCmd( CUserCmd& cmd )
{
    if ( bot_attack.GetBool() )
        cmd.buttons |= IN_ATTACK;


    if ( bot_mimic.GetInt() <= 0 )
        return false;

    // A specific target?
    if ( bot_mimic_target.GetInt() > 0 && bot_mimic_target.GetInt() == entindex() )
    {
        return false;
    }

    CBasePlayer* pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt() );
    if ( !pPlayer )
        return false;

    const CUserCmd* pPlyCmd = pPlayer->GetLastUserCommand();
    if ( !pPlyCmd )
        return false;


    cmd = *pPlyCmd;
    cmd.viewangles[PITCH] += bot_mimic_pitch_offset.GetFloat();
    cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();
    cmd.viewangles[ROLL] = 0.0f;

    // HACK
    SetEyeAngles( cmd.viewangles );

    return true;
}

ZMBotWeaponTypeRange_t CZMPlayerBot::GetWeaponType( CZMBaseWeapon* pWeapon )
{
    if ( !pWeapon )
        return BOTWEPRANGE_INVALID;

    return GetWeaponType( pWeapon->GetClassname() );
}

bool CZMPlayerBot::MustStopToShoot() const
{
    auto type = GetWeaponType( GetActiveWeapon() );

    if ( type == BOTWEPRANGE_LONGRANGE )
    {
        const char* wep = GetActiveWeapon()->GetClassname() + 10;
        if ( FStrEq( wep, "rifle" ) || FStrEq( wep, "revolver" ) )
            return true;
    }

    return false;
}

ZMBotWeaponTypeRange_t CZMPlayerBot::GetWeaponType( const char* classname )
{
    if ( !classname || !(*classname) )
        return BOTWEPRANGE_INVALID;

    // ZMRTODO: Get rid of this hardcoded stuff.

    // Skip 'weapon_zm_'
    const char* wep = classname + sizeof( "weapon_zm_" ) - 1;

    if ( FStrEq( wep, "mac10" ) )
        return BOTWEPRANGE_CLOSERANGE;
    if ( Q_strncmp( wep, "shotgun", sizeof( "shotgun" ) - 1 ) == 0 )
        return BOTWEPRANGE_CLOSERANGE;

    if ( FStrEq( wep, "rifle" ) )
        return BOTWEPRANGE_LONGRANGE;
    if ( FStrEq( wep, "pistol" ) )
        return BOTWEPRANGE_LONGRANGE;
    if ( FStrEq( wep, "revolver" ) )
        return BOTWEPRANGE_LONGRANGE;

    if ( FStrEq( wep, "improvised" ) )
        return BOTWEPRANGE_MELEE;
    if ( FStrEq( wep, "sledge" ) )
        return BOTWEPRANGE_MELEE;
    if ( FStrEq( wep, "fistscarry" ) )
        return BOTWEPRANGE_MELEE;

    if ( FStrEq( wep, "molotov" ) )
        return BOTWEPRANGE_THROWABLE;

    return BOTWEPRANGE_INVALID;
}

bool CZMPlayerBot::HasWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const
{
    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        CBaseCombatWeapon* pWep = m_hMyWeapons.Get( i );
        if ( pWep && GetWeaponType( ToZMBaseWeapon( pWep ) ) == wepType )
        {
            return true;
        }
    }
    
    return false;
}

bool CZMPlayerBot::HasEquippedWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();

    return ( pWep && GetWeaponType( pWep ) == wepType );
}

bool CZMPlayerBot::EquipWeaponOfType( ZMBotWeaponTypeRange_t wepType )
{
    CZMBaseWeapon* pWep = FindWeaponOfType( wepType );
    if ( !pWep )
        return false;

    if ( pWep == m_hActiveWeapon.Get() )
        return true;


    Weapon_Switch( pWep );
    return pWep == m_hActiveWeapon.Get();
}

ZMBotWeaponTypeRange_t CZMPlayerBot::GetCurrentWeaponType() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep )
        return BOTWEPRANGE_INVALID;

    return GetWeaponType( pWep );
}

bool CZMPlayerBot::WeaponHasAmmo( CZMBaseWeapon* pWeapon ) const
{
    if ( !pWeapon )
        return false;

    if ( !pWeapon->UsesPrimaryAmmo() )
        return true;

    return pWeapon->Clip1() > 0 || GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) > 0;
}

bool CZMPlayerBot::ShouldReload() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep )
        return false;

    return ( pWep->UsesClipsForAmmo1() && pWep->UsesPrimaryAmmo() && pWep->Clip1() == 0 );
}

bool CZMPlayerBot::HasAnyEffectiveRangeWeapons() const
{
    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        CZMBaseWeapon* pWep = ToZMBaseWeapon( m_hMyWeapons.Get( i ) );
        if ( !pWep )
            continue;


        ZMBotWeaponTypeRange_t wepType = GetWeaponType( pWep );
        if ( wepType != BOTWEPRANGE_CLOSERANGE && wepType != BOTWEPRANGE_LONGRANGE )
            continue;

        if ( WeaponHasAmmo( pWep ) )
            return true;
    }

    return false;
}

bool CZMPlayerBot::CanReload() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep ) return false;

    if ( pWep->Clip1() < pWep->GetMaxClip1() )
        return true;

    return false;
}

bool CZMPlayerBot::CanAttack() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep ) return false;

    if ( pWep->UsesPrimaryAmmo() && pWep->Clip1() <= 0 )
        return false;

    return pWep->m_flNextPrimaryAttack <= gpGlobals->curtime;
}

float CZMPlayerBot::GetOptimalAttackDistance() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep ) return FLT_MAX;

    // ZMRTODO: Get rid of this hardcoded stuff.

    // HACK: Melee
    if ( !pWep->UsesPrimaryAmmo() )
        return 32.0f;


    // Skip 'weapon_zm_'
    const char* wep = pWep->GetClassname() + sizeof( "weapon_zm_" ) - 1;

    if ( FStrEq( wep, "rifle" ) || FStrEq( wep, "revolver" ) )
        return 768.0f;
    if ( FStrEq( wep, "shotgun_sporting" ) )
        return 150.0f;
    if ( FStrEq( wep, "shotgun" ) )
        return 100.0f;
    if ( FStrEq( wep, "mac10" ) )
        return 200.0f;

    return 400.0f;
}

float CZMPlayerBot::GetMaxAttackDistance() const
{
    CZMBaseWeapon* pWep = GetActiveWeapon();
    if ( !pWep ) return FLT_MAX;

    // ZMRTODO: Get rid of this hardcoded stuff.

    // HACK: Melee
    if ( !pWep->UsesPrimaryAmmo() )
        return 50.0f;


    // Skip 'weapon_zm_'
    const char* wep = pWep->GetClassname() + sizeof( "weapon_zm_" ) - 1;

    if ( FStrEq( wep, "rifle" ) || FStrEq( wep, "revolver" ) )
        return 1024.0f;
    if ( FStrEq( wep, "shotgun_sporting" ) )
        return 400.0f;
    if ( FStrEq( wep, "shotgun" ) )
        return 256.0f;
    if ( FStrEq( wep, "mac10" ) )
        return 512.0f;

    return 768.0f;
}

CZMBaseWeapon* CZMPlayerBot::FindWeaponOfType( ZMBotWeaponTypeRange_t wepType ) const
{
    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        CZMBaseWeapon* pWep = ToZMBaseWeapon( m_hMyWeapons.Get( i ) );
        if ( pWep && GetWeaponType( pWep ) == wepType )
        {
            return pWep;
        }
    }

    return nullptr;
}

CON_COMMAND( bot, "" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }


    CZMPlayerBot::CreateZMBot();
}

CON_COMMAND( zm_bot, "" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
    {
        return;
    }


    const char* name = args.Arg( 1 );

    CZMPlayerBot::CreateZMBot( name );
}
