#pragma once


//#include "cbase.h"
#include <tier1/utlvector.h>
#include <mathlib/vector.h>
#include "igamesystem.h"


// This is the absolute maximum bullets the player can shoot.
#define ZM_USERCMD_MAX_BULLETS_PER_SHOT     8
#define ZM_USERCMD_MAX_HITS                  12
#define ZM_USERCMD_MAX_HITS_BITS             ( Q_log2( ZM_USERCMD_MAX_HITS ) + 1 )
#define ZM_NUMHITS_BITS                     ( Q_log2( ZM_USERCMD_MAX_BULLETS_PER_SHOT ) + 1 )
// Number of bits we're going to reserve for the hitgroup in the network message.
#define ZM_HITGROUP_BITS                    ( Q_log2( HITGROUP_GEAR ) + 1 )


struct ZMUserCmdHitData_t
{
    void Merge( const ZMUserCmdHitData_t& hit );

    void WriteTo( bf_write* buf ) const;
    void ReadFrom( bf_read* buf );


    int entindex;
    int nHits;
    int hitgroups[ZM_USERCMD_MAX_BULLETS_PER_SHOT];
};

typedef CUtlVector<ZMUserCmdHitData_t> ZMUserCmdHitList_t;

#ifdef GAME_DLL
struct ZMServerWepData_t;
class CZMPlayer;
#endif

class CZMUserCmdSystem : public CAutoGameSystem
{
public:
    CZMUserCmdSystem();
    ~CZMUserCmdSystem();



#ifdef CLIENT_DLL
    void ClearDamage();


    void WriteToCmd( CUserCmd& cmd );
    void AddDamage( ZMUserCmdHitData_t hit );
#else
    int ApplyDamage(
        CZMPlayer* pPlayer,
        const ZMServerWepData_t& dmgdata,
        const ZMUserCmdHitList_t& list );
#endif

    bool UsesClientsideDetection( CBaseEntity* pEnt ) const;

protected:
#ifdef CLIENT_DLL
    int FindDamageByEntity( int entindex ) const;
    ZMUserCmdHitData_t* FindDamageByEntity( int entindex );
#endif

private:
#ifdef CLIENT_DLL
    ZMUserCmdHitList_t m_vHitEnts;
#endif
};

extern CZMUserCmdSystem g_ZMUserCmdSystem;

