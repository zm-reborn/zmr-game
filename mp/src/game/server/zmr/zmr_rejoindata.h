#pragma once

#include "igamesystem.h"
#include "GameEventListener.h"

#include "zmr_player.h"


//
class CZMRejoinData
{
public:
    CZMRejoinData();
    virtual ~CZMRejoinData();

    virtual const char* GetDataName() const { return "N/A"; }

    virtual bool    DeleteOnMapChange() const { return false; }
    virtual int     GetExpirationTime() const { return 0; }



    virtual bool ShouldKeepData( bool bMapChange, time_t leavetime );

    virtual void RestoreData( CZMPlayer* pPlayer ) = 0;
};
//


//
class CZMRejoinPlayer
{
public:
    CZMRejoinPlayer( CSteamID id );
    virtual ~CZMRejoinPlayer();


    void OnPlayerJoin( CZMPlayer* pPlayer );
    void CheckDataExpiration( bool bMapChange );


    template<typename EachFunc>
    void ForEachData( EachFunc func )
    {
        int len = m_vRejoinData.Count();
        for ( int i = 0; i < len; i++ )
        {
            func( m_vRejoinData[i] );
        }
    }



    bool IsSamePlayer( const CSteamID& id );

    int GetDataCount() const { return m_vRejoinData.Count(); }

    CSteamID GetSteamID() const { return m_SteamID; }
    time_t GetLeaveTime() const { return m_iLeaveTime; }

    int AddData( CZMRejoinData* pData );

private:
    CUtlVector<CZMRejoinData*> m_vRejoinData;
    CSteamID m_SteamID;
    time_t m_iLeaveTime;
};
//


//
class CZMRejoinListener
{
public:
    CZMRejoinListener();
    virtual ~CZMRejoinListener();

    virtual CZMRejoinData* OnPlayerLeave( CZMPlayer* pPlayer ) { return nullptr; }
};
//


//
class CZMRejoinDataSystem
{
public:
    CZMRejoinDataSystem();
    virtual ~CZMRejoinDataSystem();

    void OnPlayerJoin( CZMPlayer* pPlayer );
    void OnPlayerLeave( CZMPlayer* pPlayer );
    void OnLevelShutdown();


    void AddListener( CZMRejoinListener* pListener );
    bool RemoveListener( CZMRejoinListener* pListener );
    void SaveData( CZMPlayer* pPlayer, CZMRejoinData* pData );


    void PrintRejoinData();

protected:
    void CheckDataExpiration( bool bMapChange );

    int FindPlayer( const CSteamID& id ) const;

private:
    template<typename EachFunc>
    void ForEachListener( EachFunc func )
    {
        int len = m_vListeners.Count();
        for ( int i = 0; i < len; i++ )
        {
            func( m_vListeners[i] );
        }
    }

    template<typename EachFunc>
    void ForEachPlayer( EachFunc func )
    {
        int len = m_vPlayerData.Count();
        for ( int i = 0; i < len; i++ )
        {
            func( m_vPlayerData[i] );
        }
    }


    CUtlVector<CZMRejoinListener*> m_vListeners;
    CUtlVector<CZMRejoinPlayer*> m_vPlayerData;
};

extern CZMRejoinDataSystem* GetZMRejoinSystem();
