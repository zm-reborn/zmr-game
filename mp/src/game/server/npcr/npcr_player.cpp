#include "cbase.h"
#include "in_buttons.h"

#include "npcr_motor_player.h"
#include "npcr_manager.h"
#include "npcr_player.h"


#ifdef NPCR_BOT_CMDS
ConVar bot_attack( "bot_attack", "0" );
ConVar bot_mimic( "bot_mimic", "0" );
ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0" );

CON_COMMAND( bot_sendcmd, "Sends command to all bot players" )
{
    if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;


    NPCR::g_NPCManager.ForEachNPC( [ &args ]( NPCR::CBaseNPC* pNPC )
    {
        CBasePlayer* pPlayer = ToBasePlayer( pNPC->GetCharacter() );
        if ( pPlayer )
        {
            pPlayer->ClientCommand( args );
        }

        return false;
    } );
}
#endif

ConVar bot_mimic_pitch_offset( "bot_mimic_pitch_offset", "0" );
// A specific target?
ConVar bot_mimic_target( "bot_mimic_target", "0", 0, "A specific bot that will mimic the actions" );


NPCR::CPlayerCmdHandler::CPlayerCmdHandler( CBaseCombatCharacter* pNPC ) : NPCR::CBaseNPC( pNPC )
{
}

NPCR::CPlayerCmdHandler::~CPlayerCmdHandler()
{
}

NPCR::CPlayerMotor* NPCR::CPlayerCmdHandler::GetPlayerMotor() const
{
    return static_cast<CPlayerMotor*>( CBaseNPC::GetMotor() );
}

NPCR::CBaseMotor* NPCR::CPlayerCmdHandler::CreateMotor()
{
    return new CPlayerMotor( this );
}

DEFINE_BTN( NPCR::CPlayerCmdHandler, PressFire1 )
DEFINE_BTN( NPCR::CPlayerCmdHandler, PressFire2 )
DEFINE_BTN( NPCR::CPlayerCmdHandler, PressDuck )
DEFINE_BTN( NPCR::CPlayerCmdHandler, PressReload )
DEFINE_BTN( NPCR::CPlayerCmdHandler, PressUse )

void NPCR::CPlayerCmdHandler::BuildPlayerCmd( CUserCmd& cmd )
{
    HANDLE_BTN( PressFire1, IN_ATTACK, cmd )
    HANDLE_BTN( PressFire2, IN_ATTACK2, cmd )
    HANDLE_BTN( PressDuck, IN_DUCK, cmd )
    HANDLE_BTN( PressReload, IN_RELOAD, cmd )
    HANDLE_BTN( PressUse, IN_USE, cmd )


    // Movement
    // ZMRTODO: Build the movement direction here instead of in the motor?
    // The bot angles here vs in the motor::Update() may be drastically different.
    CPlayerMotor* pMotor = GetPlayerMotor();

    Vector dir = pMotor->GetMoveDir();
    cmd.forwardmove = dir.x * 400.0f;
    cmd.sidemove = dir.y * 400.0f;
    cmd.upmove = dir.z * 400.0f;

    // Also build the the buttons from move in case some thirdparty plugins used it.
    if ( cmd.forwardmove > 0.0f )
        cmd.buttons |= IN_FORWARD;
    else if ( cmd.forwardmove < 0.0f )
        cmd.buttons |= IN_BACK;

    if ( cmd.sidemove > 0.0f )
        cmd.buttons |= IN_MOVELEFT;
    else if ( cmd.sidemove < 0.0f )
        cmd.buttons |= IN_MOVERIGHT;


    cmd.viewangles = GetEyeAngles();
}
