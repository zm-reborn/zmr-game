#include "cbase.h"

#include "zmr_gamerules.h"
#include "zmr_rejoindata.h"


extern ConVar zm_sv_debug_rejoindata;


// Whenever a player disconnects, save their rounds.
// If the round is the same, disallow late spawn.

class CZMRejoinLateSpawnData : public CZMRejoinData
{
private:
    int m_nLastRoundPlayed;

public:
    CZMRejoinLateSpawnData( int iRound ) : m_nLastRoundPlayed( iRound )
    {
    }

    virtual const char* GetDataName() const OVERRIDE { return "Late spawn"; }

    virtual bool DeleteOnMapChange() const OVERRIDE { return true; }

    virtual bool Validate() const
    {
        auto* pRules = ZMRules();
        Assert( pRules );

        bool res = pRules->GetRoundsPlayed( true ) != m_nLastRoundPlayed;
        
        if ( zm_sv_debug_rejoindata.GetBool() )
        {
            DevMsg( "Validating player's late spawning. Res: %i\n", res );
        }

        return res;
    }
};


class CZMRejoinLateSpawn : public CZMRejoinListener
{
public:
    virtual CZMRejoinData* OnPlayerLeave( CZMPlayer* pPlayer ) OVERRIDE
    {
        auto* pRules = ZMRules();
        Assert( pRules );

        int nRounds = pRules->GetRoundsPlayed( true );
        return new CZMRejoinLateSpawnData( nRounds );
    }
};


static CZMRejoinLateSpawn g_ZMRejoinLateSpawn;
