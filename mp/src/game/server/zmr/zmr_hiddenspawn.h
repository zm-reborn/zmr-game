#pragma once


class CZMHiddenSpawnSystem
{
public:
    CZMHiddenSpawnSystem();
    ~CZMHiddenSpawnSystem();


    bool Spawn( ZombieClass_t zclass, CZMPlayer* pZM, const Vector& pos );


    float   GetMinimumDistance() const;
    int     GetResourcesMax( ZombieClass_t zclass ) const;

    bool CanSee( CZMPlayer* pSurvivor, const Vector& zombiepos ) const;
    bool CanSpawnClass( ZombieClass_t zclass ) const;


    int ComputeResourceCost( ZombieClass_t zclass, float closestZombieDist ) const;

private:
    void Trace( CZMPlayer* pSurvivor, const Vector& endpos, trace_t& tr ) const;
};


extern CZMHiddenSpawnSystem g_ZMHiddenSpawn;
