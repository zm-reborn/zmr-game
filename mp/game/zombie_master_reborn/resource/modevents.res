//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local  : any data, but not networked to clients
//
// following key names are reserved:
//   local      : if set to 1, event is not networked to clients
//   unreliable : networked, but unreliable
//   suppress   : never fire this event
//   time	: firing server time
//   eventid	: holds the event ID

"modevents"
{
	// ZMR
	
	// Before round end occurs.
	"round_end"
	{
		// Reason id. See ZMRoundEndReason_t, github.com/zm-reborn/zmr-game/blob/master/mp/src/game/shared/zmr/zmr_shareddefs.h
		"reason"	"short"
	}
	
	// When round has ended and we are waiting for the round intermission to end to restart the round. See below.
	"round_end_post"
	{
		// Reason id. See ZMRoundEndReason_t, github.com/zm-reborn/zmr-game/blob/master/mp/src/game/shared/zmr/zmr_shareddefs.h
		"reason"	"short"
	}
	
	// Right before round will get restarted. (still technically previous round)
	// Not broadcasted to clients.
	"round_restart_pre"
	{
	}
	
	// Right after round has been restarted. (players have been spawned, etc.)
	"round_restart_post"
	{
	}
	
	"voicemenu_use"
	{
		"userid"	"short" // User ID of the player saying shit
		"voiceline"	"short" // The index of voice line
		
		// The position of the player. This is used if the player does not exist on the client.
		"pos_x"	"float"
		"pos_y"	"float"
		"pos_z"	"float"
		
		"seed"	"short" // Seed for the random sound to be played. Negative value will be ignored.
	}
	
	// Tell clients to reload their weapon configs.
	// Makes editing configs very easy.
	"reload_weapon_config"
	{
	}
	
	
	
	"nav_blocked"
	{
		"area"	"long"
		"blocked"	"short"
	}
	
	"player_death"				// a game event, name may be 32 charaters long
	{
		"userid"	"short"   	// user ID who died				
		"attacker"	"short"	 	// user ID who killed
		"weapon"	"string" 	// weapon name killed used 
	}
	
	// Unused(?)
	"spec_target_updated"
	{
	}
	
	// Unused(?)
	"achievement_earned"
	{
		"player"	"byte"		// entindex of the player
		"achievement"	"short"		// achievement ID
	}
}
