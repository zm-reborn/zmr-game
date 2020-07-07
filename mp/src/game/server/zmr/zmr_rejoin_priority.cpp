#include "cbase.h"

#include "zmr_rejoindata.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_sv_rejoin_priority_expire( "zm_sv_rejoin_priority_expire", "800", FCVAR_NOTIFY, "How long in seconds ZM pick priority is kept in cache." );

extern ConVar zm_sv_debug_rejoindata;


// Whenever a player disconnects, save their ZM pick priority.
// This may be due to map change or just leaving.

class CZMRejoinPriorityData : public CZMRejoinData
{
private:
    int m_nPriority;

public:
    CZMRejoinPriorityData( int priority ) : m_nPriority( priority )
    {
    }

    virtual const char* GetDataName() const OVERRIDE { return "Pick priority"; }

    virtual int GetExpirationTime() const OVERRIDE { return zm_sv_rejoin_priority_expire.GetInt(); }

    virtual void RestoreData( CZMPlayer* pPlayer ) OVERRIDE
    {
        pPlayer->SetPickPriority( m_nPriority );

        if ( zm_sv_debug_rejoindata.GetBool() )
        {
            DevMsg( "Restoring player's %i pick priority to %i!\n", pPlayer->entindex(), m_nPriority );
        }
    }
};


class CZMRejoinPriority : public CZMRejoinListener
{
public:
    virtual CZMRejoinData* OnPlayerLeave( CZMPlayer* pPlayer ) OVERRIDE
    {
        return new CZMRejoinPriorityData( pPlayer->GetPickPriority() );
    }
};


static CZMRejoinPriority g_ZMRejoinPriority;
