#include "cbase.h"
#include <ctime>

#include "zmr_rejoindata.h"


ConVar zm_sv_debug_rejoindata( "zm_sv_debug_rejoindata", "0" );



//
CZMRejoinData::CZMRejoinData()
{
}

CZMRejoinData::~CZMRejoinData()
{
}

bool CZMRejoinData::ShouldKeepData( bool bMapChange, time_t leavetime )
{
    int expire = GetExpirationTime();

    if ( expire > 0 )
    {
        time_t curtime;
        time( &curtime );

        time_t exp = leavetime + (time_t)expire;

        if ( zm_sv_debug_rejoindata.GetBool() )
        {
            DevMsg( "Expiring in %lld seconds...\n",
                exp > curtime ? (exp - curtime) : -1 );
        }

        if ( exp <= curtime )
            return false;
    }

    if ( DeleteOnMapChange() && bMapChange )
        return false;

    return true;
}
//



//
CZMRejoinPlayer::CZMRejoinPlayer( CSteamID id ) : m_SteamID( id )
{
    time_t curtime;
    time( &curtime );
    m_iLeaveTime = curtime;
}

CZMRejoinPlayer::~CZMRejoinPlayer()
{
    m_vRejoinData.PurgeAndDeleteElements();
}

bool CZMRejoinPlayer::IsSamePlayer( const CSteamID& id )
{
    return m_SteamID.ConvertToUint64() == id.ConvertToUint64();
}

void CZMRejoinPlayer::OnPlayerJoin( CZMPlayer* pPlayer )
{
    time_t leavetime = GetLeaveTime();
    ForEachData( [ pPlayer, &leavetime ]( CZMRejoinData* pData )
    {
        if ( pData->ShouldKeepData( false, leavetime ) )
        {
            pData->RestoreData( pPlayer );
        }
    } );
}

void CZMRejoinPlayer::CheckDataExpiration( bool bMapChange )
{
    for ( int i = 0; i < m_vRejoinData.Count(); i++ )
    {
        CZMRejoinData* pData = m_vRejoinData[i];

        if ( !pData->ShouldKeepData( bMapChange, m_iLeaveTime ) )
        {
            delete m_vRejoinData[i];
            m_vRejoinData.Remove( i );
            --i;
        }
    }
}

int CZMRejoinPlayer::AddData( CZMRejoinData* pData )
{
    return m_vRejoinData.AddToTail( pData );
}
//



//
CZMRejoinListener::CZMRejoinListener()
{
    GetZMRejoinSystem()->AddListener( this );
}

CZMRejoinListener::~CZMRejoinListener()
{
}
//


//
CZMRejoinDataSystem::CZMRejoinDataSystem()
{
}

CZMRejoinDataSystem::~CZMRejoinDataSystem()
{
    FOR_EACH_VEC( m_vListeners, i )
    {
        if ( m_vListeners[i]->FreeMe() )
            delete m_vListeners[i];
    }
    m_vListeners.RemoveAll();

    m_vPlayerData.PurgeAndDeleteElements();
}

void CZMRejoinDataSystem::AddListener( CZMRejoinListener* pListener )
{
    m_vListeners.AddToTail( pListener );
}

bool CZMRejoinDataSystem::RemoveListener( CZMRejoinListener* pListener )
{
    return m_vListeners.FindAndRemove( pListener );
}

void CZMRejoinDataSystem::OnPlayerJoin( CZMPlayer* pPlayer )
{
    if ( pPlayer->IsBot() ) return;


    CSteamID id;
    if ( !pPlayer->GetSteamID( &id ) )
    {
        DevMsg( "Rejoin - Player has no Steam Id! Can't load data!\n" );
        return;
    }


    if ( zm_sv_debug_rejoindata.GetBool() )
    {
        DevMsg( "Rejoin - Player %i has joined.\n", pPlayer->entindex() );
    }



    int i = FindPlayer( id );

    if ( i != -1 )
    {
        m_vPlayerData[i]->OnPlayerJoin( pPlayer );

        delete m_vPlayerData[i];
        m_vPlayerData.Remove( i );
    }
}

void CZMRejoinDataSystem::OnPlayerLeave( CZMPlayer* pPlayer )
{
    if ( pPlayer->IsBot() ) return;


    if ( zm_sv_debug_rejoindata.GetBool() )
    {
        DevMsg( "Rejoin - Player %i has left.\n", pPlayer->entindex() );
    }


    ForEachListener( [ pPlayer, this ]( CZMRejoinListener* pListener )
    {
        CZMRejoinData* pData = pListener->OnPlayerLeave( pPlayer );
        if ( !pData ) return;


        SaveData( pPlayer, pData );
    } );
}

void CZMRejoinDataSystem::OnLevelShutdown()
{
    CheckDataExpiration( true );
}

void CZMRejoinDataSystem::SaveData( CZMPlayer* pPlayer, CZMRejoinData* pData )
{
    if ( pPlayer->IsBot() ) return;


    CSteamID id;
    if ( !pPlayer->GetSteamID( &id ) )
    {
        DevMsg( "Rejoin - Player has no Steam Id! Can't save data!\n" );
        return;
    }


    int i = FindPlayer( id );
    if ( i <= -1 )
    {
        i = m_vPlayerData.AddToTail( new CZMRejoinPlayer( id ) );
    }

    m_vPlayerData[i]->AddData( pData );


    if ( zm_sv_debug_rejoindata.GetBool() )
    {
        DevMsg( "Rejoin - Saved %s for player %llu\n", pData->GetDataName(), id.ConvertToUint64() );
    }
}

const CZMRejoinData* CZMRejoinDataSystem::FindPlayerData( CZMPlayer* pPlayer, const char* pszDataName ) const
{
    int i = FindPlayer( pPlayer );
    if ( i == -1 )
        return nullptr;


    CZMRejoinData* pOut = nullptr;
    m_vPlayerData[i]->ForEachData( [ &pOut, pszDataName ]( CZMRejoinData* pData ) {
        if ( Q_strcmp( pszDataName, pData->GetDataName() ) == 0 )
        {
            pOut = pData;
            return;
        }
    } );

    return pOut;
}

int CZMRejoinDataSystem::FindPlayer( CZMPlayer* pPlayer ) const
{
    Assert( pPlayer );

    CSteamID id;
    if ( !pPlayer->GetSteamID( &id ) )
    {
        DevMsg( "Rejoin - Player %i has no Steam Id! Can't find player!\n", pPlayer->entindex() );
        return -1;
    }

    return FindPlayer( id );
}

int CZMRejoinDataSystem::FindPlayer( const CSteamID& id ) const
{
    for ( int i = 0; i < m_vPlayerData.Count(); i++ )
    {
        if ( m_vPlayerData[i]->IsSamePlayer( id ) )
            return i;
    }

    return -1;
}

void CZMRejoinDataSystem::CheckDataExpiration( bool bMapChange )
{
    if ( zm_sv_debug_rejoindata.GetBool() )
    {
        DevMsg( "Rejoin - Checking data expiration for %i players...\n", m_vPlayerData.Count() );

        PrintRejoinData();
    }

    for ( int i = 0; i < m_vPlayerData.Count(); i++ )
    {
        CZMRejoinPlayer* pPlayerData = m_vPlayerData[i];

        pPlayerData->CheckDataExpiration( bMapChange );

        // Player has no data left, delete it.
        if ( !pPlayerData->GetDataCount() )
        {
            delete m_vPlayerData[i];
            m_vPlayerData.Remove( i );
            --i;
        }
    }
}

void CZMRejoinDataSystem::PrintRejoinData()
{
    if ( !m_vPlayerData.Count() )
    {
        Msg( "No rejoin data found\n" );
        return;
    }


    time_t curtime;
    time( &curtime );
    

    Msg( "Rejoin data:\n" );


    for ( int i = 0; i < m_vPlayerData.Count(); i++ )
    {
        time_t leavetime = m_vPlayerData[i]->GetLeaveTime();

        Msg( "  Steam ID: %llu | Left: %lld seconds ago\n",
            m_vPlayerData[i]->GetSteamID().ConvertToUint64(),
            (uint64)(curtime - leavetime) );

        m_vPlayerData[i]->ForEachData( [ &leavetime, &curtime ]( CZMRejoinData* pData )
        {
            int expire = pData->GetExpirationTime();
            time_t exp = leavetime + (time_t)expire;
            Msg( "    %s | Expires in: %lld seconds\n",
                pData->GetDataName(),
                (expire > 0 && exp > curtime) ? (exp - curtime) : -1 );
        } );
    }
}

CZMRejoinDataSystem* GetZMRejoinSystem()
{
    static CZMRejoinDataSystem s_ZMRejoinSystem;
    return &s_ZMRejoinSystem;
}
//


CON_COMMAND( zm_printrejoindata, "" )
{
    GetZMRejoinSystem()->PrintRejoinData();
}
