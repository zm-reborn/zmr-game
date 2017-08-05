#pragma once

enum
{
    ZMTEAM_INVALID = -1,
    
    ZMTEAM_UNASSIGNED,
    
    ZMTEAM_SPECTATOR,
    ZMTEAM_HUMAN,
    ZMTEAM_ZM
};

enum ZMRoundEndReason_t
{
    ZMROUND_HUMANDEAD = 0,

    ZMROUND_HUMANWIN,

    ZMROUND_HUMANLOSE,

    ZMROUND_ZMSUBMIT,

    ZMROUND_GAMEBEGIN,

    ZMROUND_NOTHING, // Use when you don't want any message.
};

enum
{
    ZMWEAPONSLOT_NONE = 0,

    //ZMWEAPONSLOT_FISTS = ( 1 << 0 ),

    ZMWEAPONSLOT_SIDEARM = ( 1 << 0 ),
    ZMWEAPONSLOT_MELEE = ( 1 << 1 ),
    ZMWEAPONSLOT_LARGE = ( 1 << 2 ),

    ZMWEAPONSLOT_EQUIPMENT = ( 1 << 3 ),

    ZMWEAPONSLOT_NOLIMIT = ( 1 << 4 ),
};

enum ZombieClass_t
{
    ZMCLASS_INVALID = -1,

    ZMCLASS_SHAMBLER,
    ZMCLASS_BANSHEE,
    ZMCLASS_HULK,
    ZMCLASS_DRIFTER,
    ZMCLASS_IMMOLATOR,

    ZMCLASS_MAX
};

/*
#define TEAMNAME_SPECTATOR          "#TeamName_Spec"
#define TEAMNAME_SURVIVOR           "#TeamName_Survivor"
#define TEAMNAME_ZM                 "#TeamName_ZombieMaster"
*/

#define SIZE_ZMAMMO_REVOLVER        6


#define ZM_POINTS_HUMAN_WIN         50
#define ZM_POINTS_ZM_WIN            10


// IN_RUN
#define IN_ZM_OBSERVERMODE          ( 1 << 12 )
