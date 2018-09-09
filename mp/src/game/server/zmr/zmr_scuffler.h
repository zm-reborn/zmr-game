#pragma once


#include "zmr_entities.h"
#include "zmr_player.h"

class CZMScufflerSystem
{
public:
    bool OnRequestScuffler( CZMPlayer* pPlayer );

protected:
    CZMEntZombieSpawn* FindClosestZombieSpawn( const Vector& pos, float flMaxDist = FLT_MAX ) const;
};
