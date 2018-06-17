#include "cbase.h"

#include "zmr/zmr_global_shared.h"
#include "zmr_util.h"


int ZMUtil::GetSelectedZombieCount( int iPlayerIndex )
{
    int num = 0;

    g_ZombieManager.ForEachSelectedZombie( iPlayerIndex, [ &num ]( CZMBaseZombie* pZombie )
    {
        ++num;
    } );

    return num;
}

void ZMUtil::MoveSelectedZombies( int iPlayerIndex, const Vector& vecPos )
{
    // Move the zombies in about the same formation they are right now.
    int nZombies = ZMUtil::GetSelectedZombieCount( iPlayerIndex );
    if ( nZombies < 1 )
        return;


    CZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
    if ( !pPlayer )
        return;


    Vector vecTracePos = vecPos;
    vecTracePos.z += 4.0f;

    if ( UTIL_PointContents( vecTracePos ) & CONTENTS_SOLID )
        return;
    

    float tolerance = nZombies * 18.0f;

    int mask = MASK_NPCSOLID & ~(CONTENTS_MOVEABLE | CONTENTS_MONSTER);


    // Get the average position and build the list of zombies
    Vector vecAvg = vec3_origin;
    CUtlVector<CZMBaseZombie*> vZombies;
    float flFurthestZombie = -1.0f;

    g_ZombieManager.ForEachSelectedZombie( iPlayerIndex, [ &vecAvg, &vZombies ]( CZMBaseZombie* pZombie )
    {
        vecAvg += pZombie->GetAbsOrigin();
        vZombies.AddToTail( pZombie );
    } );


    if ( vZombies.Count() == 1 )
    {
        vZombies[0]->Command( pPlayer, vecPos );
        return;
    }

    vecAvg /= nZombies;


    // Get the furthest unit
    FOR_EACH_VEC( vZombies, i )
    {
        Vector dir = vZombies[i]->GetAbsOrigin() - vecAvg;
        float dist = dir.Length2D();
        if ( flFurthestZombie < 0.0f || dist > flFurthestZombie )
            flFurthestZombie = dist;
    }

    if ( flFurthestZombie < 16.0f )
        flFurthestZombie = 16.0f;


    FOR_EACH_VEC( vZombies, i )
    {
        Vector dir = vZombies[i]->GetAbsOrigin() - vecAvg;
        dir.z = 0.0f;
        float flDist = dir.NormalizeInPlace();


        Vector pos = vecPos;

        if ( flDist > tolerance )
        {
            float ratio = flDist / flFurthestZombie;
            pos += dir * (ratio * tolerance);
        }
        else
        {
            pos += dir * flDist;
        }

        // Trace down to the position to get an accurate, valid position.
        // We may be on stairs or something.
        trace_t tr;
        UTIL_TraceLine( vecTracePos, pos, mask, nullptr, COLLISION_GROUP_NONE, &tr );

        vZombies[i]->Command( pPlayer, tr.endpos );
    }
}

void ZMUtil::PrintNotify( CBasePlayer* pPlayer, ZMChatNotifyType_t type, const char* msg )
{
    CSingleUserRecipientFilter filter( pPlayer );
    filter.MakeReliable();

    SendNotify( filter, type, msg );
}

void ZMUtil::PrintNotifyAll( ZMChatNotifyType_t type, const char* msg )
{
    CRecipientFilter filter;
    filter.AddAllPlayers();
    filter.MakeReliable();

    SendNotify( filter, type, msg );
}

void ZMUtil::SendNotify( IRecipientFilter& filter, ZMChatNotifyType_t type, const char* msg )
{
    if ( filter.GetRecipientCount() < 1 )
        return;


    UserMessageBegin( filter, "ZMChatNotify" );
        WRITE_BYTE( type );
        WRITE_STRING( msg );
    MessageEnd();
}
