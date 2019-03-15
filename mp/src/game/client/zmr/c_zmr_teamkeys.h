#pragma once


#define KEYCONFIG_NAME_ZM           "keys_zm"
#define KEYCONFIG_NAME_SURVIVOR     "keys_survivor"
#define KEYCONFIG_NAME_SPEC         "keys_spectator"

#define KEYCONFIG_ZM                "cfg/"KEYCONFIG_NAME_ZM".cfg"
#define KEYCONFIG_SURVIVOR          "cfg/"KEYCONFIG_NAME_SURVIVOR".cfg"
#define KEYCONFIG_SPEC              "cfg/"KEYCONFIG_NAME_SPEC".cfg"

#define KEYCONFIG_ZM_DEF            "cfg/"KEYCONFIG_NAME_ZM"_default.cfg"
#define KEYCONFIG_SURVIVOR_DEF      "cfg/"KEYCONFIG_NAME_SURVIVOR"_default.cfg"
#define KEYCONFIG_SPEC_DEF          "cfg/"KEYCONFIG_NAME_SPEC"_default.cfg"



struct zmkeydata_t
{
    ButtonCode_t key;
    char cmd[128];
};

enum ZMKeyTeam_t
{
    KEYTEAM_NEUTRAL = 0, // It can be used by any team
    KEYTEAM_SURVIVOR,
    KEYTEAM_ZM,
	KEYTEAM_SPEC,
};


typedef CUtlVector<zmkeydata_t*> zmkeydatalist_t;

class CZMTeamKeysConfig
{
public:
    CZMTeamKeysConfig();
    ~CZMTeamKeysConfig();


    static bool IsZMCommand( const char* cmd );
    static bool IsSurvivorCommand( const char* cmd );
    static bool IsSpectatorCommand( const char* cmd );
    static bool IsNeutralCommand( const char* cmd );

    static ZMKeyTeam_t GetCommandType( const char* cmd );


    static bool IsKeyConflicted( ZMKeyTeam_t team1, ZMKeyTeam_t team2 );

    static bool LoadConfigByTeam( ZMKeyTeam_t team, zmkeydatalist_t& list );
    static bool LoadDefaultConfigByTeam( ZMKeyTeam_t team, zmkeydatalist_t& list );

    static bool ParseConfig( const char* cfg, zmkeydatalist_t& list );
    static bool SaveConfig( const char* cfg, const zmkeydatalist_t& list );

    // Force survivor config execution no matter what.
    static void ExecuteTeamConfig( bool bForce = false );
    static void ExecuteTeamConfig( int iTeam );
    
    static const char* TeamNumberToConfigPath( int iTeam, bool bDefault = false );
    static const char* TeamNumberToConfigName( int iTeam, bool bDefault = false );


    static zmkeydata_t* FindKeyDataFromList( const char* cmd, zmkeydatalist_t& list );
    static zmkeydata_t* FindKeyDataFromListByKey( ButtonCode_t key, zmkeydatalist_t& list );
};
