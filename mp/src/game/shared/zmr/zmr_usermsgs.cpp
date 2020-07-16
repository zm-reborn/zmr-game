#include "cbase.h"
#include "voice_common.h"
#include "usermessages.h"
#include <haptics/haptic_msgs.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RegisterZMUserMessages( void )
{
    usermessages->Register( "ZMBuildMenuUpdate", -1 ); // 1 * spawn ehandle, 5 * queue bytes
	usermessages->Register( "ZMManiMenuUpdate", -1 );


    usermessages->Register( "ZMObjDisplay", -1 );
    usermessages->Register( "ZMObjUpdate", -1 );
    usermessages->Register( "ZMObjUpdateLine", -1 );
    
    usermessages->Register( "ZMCenterText", -1 );
    usermessages->Register( "ZMTooltip", -1 );

    usermessages->Register( "ZMChatNotify", -1 );


    // From vote_manager
    usermessages->Register( "CallVoteFailed", -1 );
    usermessages->Register( "VoteStart", -1 );
    usermessages->Register( "VotePass", -1 );
    usermessages->Register( "VoteFailed", -1 );
    usermessages->Register( "VoteSetup", -1 );
}



void RegisterUserMessages()
{
	usermessages->Register( "Geiger", 1 );
	//usermessages->Register( "Train", 1 );
	usermessages->Register( "HudText", -1 );
	usermessages->Register( "SayText", -1 );
	usermessages->Register( "SayText2", -1 );
	usermessages->Register( "TextMsg", -1 );
	usermessages->Register( "HudMsg", -1 );
	usermessages->Register( "ResetHUD", 1);		// called every respawn
	usermessages->Register( "GameTitle", 0 );
	usermessages->Register( "ItemPickup", -1 );
	usermessages->Register( "ShowMenu", -1 );
	usermessages->Register( "Shake", 13 );
	usermessages->Register( "Fade", 10 );
	usermessages->Register( "VGUIMenu", -1 );	// Show VGUI menu
	usermessages->Register( "Rumble", 3 );	// Send a rumble to a controller
	usermessages->Register( "Damage", 18 );		// BUG: floats are sent for coords, no variable bitfields in hud & fixed size Msg
	usermessages->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	usermessages->Register( "RequestState", 0 );
	usermessages->Register( "CloseCaption", -1 ); // Show a caption (by string id number)(duration in 10th of a second)
	usermessages->Register( "HintText", -1 );	// Displays hint text display
	usermessages->Register( "KeyHintText", -1 );	// Displays hint text display
	usermessages->Register( "AmmoDenied", 2 );
	usermessages->Register( "CreditsMsg", 1 );
	usermessages->Register( "LogoTimeMsg", 4 );
	usermessages->Register( "AchievementEvent", -1 );

#ifndef _X360
	// NVNT register haptic user messages
	RegisterHapticMessages();
#endif

    RegisterZMUserMessages();
}
