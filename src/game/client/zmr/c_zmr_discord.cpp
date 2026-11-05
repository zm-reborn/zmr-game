#include "cbase.h"

#include <ctime>

#ifdef WIN32
#define _WINREG_
#undef ReadConsoleInput
#undef INVALID_HANDLE_VALUE
#undef GetCommandLine
#endif
#include <discord/discord.h>


#include "c_zmr_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static const discord::ClientId DISCORD_APP_ID = 551073398789505065;

class CZMDiscordSystem : public CAutoGameSystemPerFrame
{
public:
    CZMDiscordSystem();
    ~CZMDiscordSystem();


    virtual void PostInit() override;

    virtual void LevelInitPostEntity() override;
    virtual void LevelShutdownPostEntity() override;

    virtual void Update( float frametime ) override;

private:
    void PresenceEmpty();
    void PresenceInGame();
    void UpdateGameStartTime();
    void InitDiscord();

    int GetPlayerCount() const;

    static bool IsInGame();


    float m_flNextDiscordUpdateTime;
    int m_nLastPlayerCount;
    uint64_t m_GameStartTime;

    discord::Core* m_pDiscordCore;
};

static CZMDiscordSystem g_ZMDiscordSystem;

//
//
//
CZMDiscordSystem::CZMDiscordSystem() : CAutoGameSystemPerFrame( "ZMDiscordSystem" )
{
    m_flNextDiscordUpdateTime = 0.0f;
    m_nLastPlayerCount = -1;

    m_pDiscordCore = nullptr;
}

CZMDiscordSystem::~CZMDiscordSystem()
{
}

void CZMDiscordSystem::PostInit()
{
    InitDiscord();

    UpdateGameStartTime();
}

void CZMDiscordSystem::LevelInitPostEntity()
{
    m_nLastPlayerCount = -1;
    m_flNextDiscordUpdateTime = 0.0f;
}

void CZMDiscordSystem::LevelShutdownPostEntity()
{
    m_nLastPlayerCount = -1;
    m_flNextDiscordUpdateTime = 0.0f;
}

void CZMDiscordSystem::Update( float frametime )
{
    // Discord not available.
    if ( !m_pDiscordCore ) return;

    m_pDiscordCore->RunCallbacks();

    if ( m_flNextDiscordUpdateTime <= gpGlobals->realtime )
    {
        if ( IsInGame() )
        {
            PresenceInGame();
        }
        else
        {
            PresenceEmpty();
        }

        m_flNextDiscordUpdateTime = gpGlobals->realtime + 0.2f;
    }
}

int CZMDiscordSystem::GetPlayerCount() const
{
    if ( !g_PR )
        return 0;

    int nPlayers = 0;
    for ( int i = 0; i < gpGlobals->maxClients; i++ )
    {
        if ( g_PR->IsConnected( i ) )
            ++nPlayers;
    }

    return nPlayers;
}

bool CZMDiscordSystem::IsInGame()
{
    return engine->IsInGame() && !engine->IsLevelMainMenuBackground();
}

void CZMDiscordSystem::PresenceEmpty()
{
    // Don't bother updating again.
    if ( m_nLastPlayerCount == 0 )
        return;


    discord::Activity activity {};
    activity.SetState( "In Menu" );
    activity.GetAssets().SetLargeImage( "zmrmain" );
    activity.GetTimestamps().SetStart( m_GameStartTime );
    m_pDiscordCore->ActivityManager().UpdateActivity( activity, []( discord::Result result ) {} );

    m_nLastPlayerCount = 0;
}

void CZMDiscordSystem::PresenceInGame()
{
    int nPlayers = GetPlayerCount();


    // Don't bother updating if we have the same data.
    if ( nPlayers == m_nLastPlayerCount )
        return;



    if ( !nPlayers ) // This shouldn't be possible.
    {
        PresenceEmpty();
        return;
    }
    

    int nMaxPlayers = gpGlobals->maxClients;

    char details[192];
    char mapname[128];
    Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );


    Q_snprintf( details, sizeof( details ), "%s (%i/%i)",
        mapname,
        nPlayers,
        nMaxPlayers );

    discord::Activity activity {};
    activity.SetState( "Playing" );
    activity.SetDetails( details );
    activity.GetAssets().SetLargeImage( "zmrmain" );
    activity.GetTimestamps().SetStart( m_GameStartTime );
    m_pDiscordCore->ActivityManager().UpdateActivity( activity, []( discord::Result result ) {} );

    m_nLastPlayerCount = nPlayers;
}

void CZMDiscordSystem::UpdateGameStartTime()
{
    time_t curtime;
    time( &curtime );

    m_GameStartTime = curtime;
}

void CZMDiscordSystem::InitDiscord()
{
    auto result = discord::Core::Create( DISCORD_APP_ID, DiscordCreateFlags_NoRequireDiscord, &m_pDiscordCore );
    if ( result != discord::Result::Ok ) {
        Warning( "Failed to init Discord rich presence. Returned result %i.\n", result );
    }
}
