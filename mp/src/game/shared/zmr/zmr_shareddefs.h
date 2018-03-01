#pragma once


#define ZMR_NAME        "Zombie Master: Reborn"
#define ZMR_VERSION     "a7"


#define ZMR_GAMEDESC    (ZMR_NAME" "ZMR_VERSION)

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

    ZMROUND_VOTERESTART,
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

enum ZombieMode_t
{
    ZOMBIEMODE_INVALID = -1,

    ZOMBIEMODE_OFFENSIVE,
    ZOMBIEMODE_DEFEND,
    ZOMBIEMODE_AMBUSH,

    ZOMBIEMODE_MAX,
};

enum Participation_t
{
    ZMPART_INVALID = -1,

    ZMPART_ALLOWZM,
    ZMPART_ONLYHUMAN,
    ZMPART_ONLYSPEC,

    ZMPART_MAX
};

enum ServerParticipation_t
{
    ZMSERVPART_NOLIMIT = 0,
    ZMSERVPART_NOHUMANSONLY,
    ZMSERVPART_NOSPECONLY,
    ZMSERVPART_NOHUMANSPECONLY
};

#define ZMPARTFLAG_NOHUMANSONLY         ( 1 << 0 )
#define ZMPARTFLAG_NOSPECONLY           ( 1 << 1 )

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




#define NUM_OBJ_LINES           5
#define MAX_OBJ_LINESIZE        128

enum ObjArgType_t
{
    OBJARGTYPE_NONE = 0,

    OBJARGTYPE_INT,
    OBJARGTYPE_FLOAT,
    OBJARGTYPE_TIMER,
    OBJARGTYPE_STRING
};

enum ObjRecipient_t
{
    OBJRECIPIENT_INVALID = -1,

    OBJRECIPIENT_HUMANS,
    OBJRECIPIENT_ZM,
    OBJRECIPIENT_ACTIVATOR,
    OBJRECIPIENT_ALL,

    OBJRECIPIENT_MAX
};


enum ZMChatNotifyType_t
{
    ZMCHATNOTIFY_NORMAL = 0,
    ZMCHATNOTIFY_ZM
};


// How far items will glow if players can pick them up.
#define ITEM_GLOW_DIST_SQR          ( 162.0f * 162.0f )

#define GLOWFLAG_OCCLUDED           ( 1 << 0 )
#define GLOWFLAG_UNOCCLUDED         ( 1 << 1 )
#define GLOWFLAG_ALWAYS             ( GLOWFLAG_OCCLUDED | GLOWFLAG_UNOCCLUDED )


#define ZM_MAX_MANI_DESC         256

#define ZM_WALK_SPEED               190.0f

#define ZM_MIN_FOV          75 // Not the absolute minimum. GetMinFOV will not use this.

// m_hViewModel
#define VMINDEX_WEP         0
#define VMINDEX_HANDS       1
