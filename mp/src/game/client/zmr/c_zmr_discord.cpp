#include "cbase.h"
#include <ctime>

#include <discordrpc/discord_rpc.h>


#include "c_zmr_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define DISCORD_APP_ID          "551073398789505065"



class CZMDiscordSystem : public CAutoGameSystemPerFrame
{
public:
    CZMDiscordSystem();
    ~CZMDiscordSystem();


    virtual void PostInit() OVERRIDE;
    virtual void Shutdown() OVERRIDE;

    virtual void LevelInitPostEntity() OVERRIDE;
    virtual void LevelShutdownPostEntity() OVERRIDE;

    virtual void Update( float frametime ) OVERRIDE;


    void PresenceEmpty();
    void PresenceInGame();
    void UpdateGameStartTime();
    void InitDiscord();

    bool IsConnected() const { return m_bConnected; }


    static void SetDisconnected();
    static void SetConnected();

private:
    int GetPlayerCount() const;

    static bool IsInGame();


    float m_flNextDiscordUpdateTime;

    int m_nLastPlayerCount;

    uint64_t m_GameStartTime;

    bool m_bConnected;
};

static CZMDiscordSystem g_ZMDiscordSystem;


static void Discord_Event_Ready( const DiscordUser* request )
{
    CZMDiscordSystem::SetConnected();
}

static void Discord_Event_Disconnected( int errorCode, const char* message )
{
    CZMDiscordSystem::SetDisconnected();
}

static void Discord_Event_Error( int errorCode, const char* message )
{
    Assert( 0 );
    DevWarning( "Discord error (%i): %s", errorCode, message );
}



//
//
//
CZMDiscordSystem::CZMDiscordSystem() : CAutoGameSystemPerFrame( "ZMDiscordSystem" )
{
    m_flNextDiscordUpdateTime = 0.0f;
    m_nLastPlayerCount = -1;

    m_bConnected = false;
}

CZMDiscordSystem::~CZMDiscordSystem()
{
}

void CZMDiscordSystem::PostInit()
{
    InitDiscord();


    UpdateGameStartTime();
    PresenceEmpty();
}

void CZMDiscordSystem::Shutdown()
{
    Discord_Shutdown();
}

void CZMDiscordSystem::LevelInitPostEntity()
{
    m_nLastPlayerCount = -1;
    m_flNextDiscordUpdateTime = 0.0f;

    Update( 0.0f );
}

void CZMDiscordSystem::LevelShutdownPostEntity()
{
    m_nLastPlayerCount = -1;

    PresenceEmpty();
}

void CZMDiscordSystem::Update( float frametime )
{
    Discord_RunCallbacks();

    if ( m_flNextDiscordUpdateTime <= gpGlobals->realtime )
    {
        if ( IsConnected() )
        {
            if ( IsInGame() )
                PresenceInGame();
            else
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

void CZMDiscordSystem::SetDisconnected()
{
    g_ZMDiscordSystem.m_bConnected = false;

    g_ZMDiscordSystem.m_nLastPlayerCount = -1;
    g_ZMDiscordSystem.m_flNextDiscordUpdateTime = 0.0f;
}

void CZMDiscordSystem::SetConnected()
{
    g_ZMDiscordSystem.m_bConnected = true;

    g_ZMDiscordSystem.m_nLastPlayerCount = -1;
    g_ZMDiscordSystem.m_flNextDiscordUpdateTime = 0.0f;
}

void CZMDiscordSystem::PresenceEmpty()
{
    // Don't bother updating again.
    if ( m_nLastPlayerCount == 0 )
        return;


    DiscordRichPresence p = { 0 };
    p.startTimestamp = m_GameStartTime;
    p.largeImageKey = "zmrmain";
    

    Discord_UpdatePresence( &p );


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


    DiscordRichPresence p = { 0 };

    p.details = details;
    p.startTimestamp = m_GameStartTime;
    p.largeImageKey = "zmrmain";
    Discord_UpdatePresence( &p );


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
    DiscordEventHandlers hndlrs = { 0 };
    hndlrs.ready = Discord_Event_Ready;
    hndlrs.disconnected = Discord_Event_Disconnected;
    hndlrs.errored = Discord_Event_Error;


    Discord_Initialize( DISCORD_APP_ID, &hndlrs, 1, nullptr );
}
